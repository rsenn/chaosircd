/**
 * ws-irc-gateway - WebSocket<->IRC bridge for chaosircd, so browser clients
 * (e.g. kiwiirc.com/nextclient) that can only open ws:// or wss:// sockets
 * can reach a raw IRC listener that has no native WebSocket support.
 *
 * Each incoming WS connection gets its own onward plain TCP ("RAW" role in
 * lws terms) connection to the backend ircd. Bytes are relayed in both
 * directions, with two framing conversions in between:
 *
 *  - WS -> ircd: each WS text frame is one IRC command from the browser
 *    client; chaosircd's line reader wants CRLF-terminated lines, so a
 *    trailing "\r\n" is appended if the client didn't already send one.
 *  - ircd -> WS: the backend TCP stream has no message boundaries at all
 *    (a single read() can contain several lines, or half of one), so
 *    incoming bytes are buffered and split on "\r\n", and each complete
 *    line is sent onward as its own WS text frame.
 *
 * Once the backend ircd sends RPL_WELCOME (numeric 001) - i.e. once the
 * client has actually registered (NICK/USER, and chaosircd's servauth
 * DNS/auth/proxy checks, have finished) - the gateway injects a
 *   SPOOF <real peer IP>
 * command on the client's behalf. chaosircd's m_spoof module (see
 * modules/msg/m_spoof.c) only rewrites the client's *displayed* host
 * string; it can't be sent any earlier because the SPOOF message handler
 * is unregistered clients only (modules/msg/m_spoof.c's m_spoof_msg
 * handler table has no LCLIENT_UNKNOWN entry), and there's no point
 * anyway before servauth's own DNS lookup finished with it.
 *
 * Run:
 *   qjs ws-irc-gateway.js
 *
 * Config is via environment variables (all optional):
 *   WS_PORT      - port to listen on for WebSocket connections (default 7778)
 *   IRCD_HOST    - backend ircd host to relay to (default 127.0.0.1)
 *   IRCD_PORT    - backend ircd port, plaintext (default 6667)
 *   SSL_CERT     - PEM certificate file, enables wss:// (TLS) if both this
 *                  and SSL_KEY are set; otherwise plain ws://
 *   SSL_KEY      - PEM private key file matching SSL_CERT
 */
import { createServer, LWSMPRO_NO_MOUNT, LWS_WRITE_TEXT, LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT, toString } from 'lws';

const WS_PORT = +(process.env.WS_PORT ?? 7778);
const IRCD_HOST = process.env.IRCD_HOST ?? '127.0.0.1';
const IRCD_PORT = +(process.env.IRCD_PORT ?? 6667);
const SSL_CERT = process.env.SSL_CERT ?? '';
const SSL_KEY = process.env.SSL_KEY ?? '';

// One shared { ws, onward, ip, wsQueue, rxbuf, spoofed } object per bridged
// connection, keyed by *both* legs' wsi so either side's close/rx handler
// can find it directly.
const links = new Map();

function relayLineToWs(link, line) {
  if(!link.spoofed && /^:\S+\s+001\s/.test(line)) {
    link.spoofed = true;
    if(link.onward)
      link.onward.write(`SPOOF ${link.ip}\r\n`);
  }

  if(link.ws)
    link.ws.write(line, LWS_WRITE_TEXT);
}

function feedFromBackend(link, chunk) {
  link.rxbuf += typeof chunk === 'string' ? chunk : toString(chunk);

  let idx;

  while((idx = link.rxbuf.indexOf('\n')) !== -1) {
    let line = link.rxbuf.slice(0, idx);

    link.rxbuf = link.rxbuf.slice(idx + 1);

    if(line.endsWith('\r'))
      line = line.slice(0, -1);

    if(line.length)
      relayLineToWs(link, line);
  }
}

function teardown(wsi) {
  const link = links.get(wsi);

  if(!link)
    return;

  const other = wsi === link.ws ? link.onward : link.ws;

  links.delete(link.ws);

  if(link.onward)
    links.delete(link.onward);

  if(other && other !== wsi)
    other.close();
}

const tlsOptions =
  SSL_CERT && SSL_KEY
    ? { serverSslCert: SSL_CERT, serverSslPrivateKey: SSL_KEY, options: LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT }
    : {};

const ctx = createServer({
  port: WS_PORT,
  vhostName: 'ws-irc-gateway',
  ...tlsOptions,
  mounts: [{ mountpoint: '/', protocol: 'irc-ws', originProtocol: LWSMPRO_NO_MOUNT }],
  protocols: [
    // Must be protocol index 0: that's the vhost's default_protocol_index,
    // which is what a WS upgrade with no Sec-WebSocket-Protocol header
    // binds to (see libwebsockets/lib/roles/ws/server-ws.c). Most
    // ircd-native WebSocket clients (including kiwiirc's direct-connect
    // mode) don't send that header at all, so without this they'd
    // silently bind to an inert placeholder protocol and get dropped
    // right after the handshake (WS_SERVER_DROP_PROTOCOL).
    {
      name: 'irc-ws',
      onEstablished(wsi) {
        const ip = wsi.peer?.host ?? '0.0.0.0';
        const link = { ws: wsi, onward: null, wsQueue: [], rxbuf: '', ip, spoofed: false };

        links.set(wsi, link);

        console.log(`+ ${ip} connecting -> ${IRCD_HOST}:${IRCD_PORT}`);

        const onward = ctx.clientConnect({ address: IRCD_HOST, port: IRCD_PORT, method: 'RAW', protocol: 'irc-backend' });

        if(!onward) {
          console.log(`- ${ip}: onward connect to ${IRCD_HOST}:${IRCD_PORT} failed immediately`);
          wsi.close();
          return;
        }

        link.onward = onward;
        links.set(onward, link);
      },
      onReceive(wsi, data) {
        const link = links.get(wsi);

        if(!link)
          return;

        let text = typeof data === 'string' ? data : toString(data);

        text = text.replace(/\r?\n?$/, '') + '\r\n';

        if(link.onward)
          link.onward.write(text);
        else
          link.wsQueue.push(text);
      },
      onClosed(wsi) {
        const link = links.get(wsi);

        if(link)
          console.log(`- ${link.ip} disconnected`);

        teardown(wsi);
      },
    },

    // The onward leg: our own plain TCP connection to the backend ircd.
    {
      name: 'irc-backend',
      onRawConnected(onward) {
        const link = links.get(onward);

        if(!link)
          return;

        for(const chunk of link.wsQueue) onward.write(chunk);

        link.wsQueue.length = 0;
      },
      onRawRx(onward, data) {
        const link = links.get(onward);

        if(link)
          feedFromBackend(link, data);
      },
      onRawClose(onward) {
        teardown(onward);
      },
      onClientConnectionError(onward, msg) {
        console.log(`backend connection failed: ${msg}`);
        teardown(onward);
      },
    },
  ],
});

console.log(`ws-irc-gateway: listening on ${SSL_CERT ? 'wss' : 'ws'}://0.0.0.0:${WS_PORT}/ -> ${IRCD_HOST}:${IRCD_PORT}`);
