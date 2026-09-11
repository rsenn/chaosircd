/* chaosircd - Chaoz's IRC daemon daemon
 *
 * Copyright (C) 2026  Roman Senn <roman.l.senn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* -------------------------------------------------------------------------- *
 * lc_lws - in-process WebSocket<->IRC gateway.                               *
 *                                                                            *
 * This is the ONLY source file in the codebase allowed to #include          *
 * <libwebsockets.h> or otherwise touch libwebsockets - everything it needs  *
 * from the rest of chaosircd goes through ordinary hook/API calls, so the   *
 * dependency stays fully opt-in (see BUILD_LC_LWS in the top-level          *
 * CMakeLists.txt, off by default) and fully contained to this one module.   *
 *                                                                            *
 * Shape, top to bottom:                                                     *
 *                                                                            *
 *  1. lc_lws_parse_hook(), on lclient_parse() (a hook point that already    *
 *     exists, fires with every raw untokenized line before anything         *
 *     assumes it's IRC): sniffs an LCLIENT_UNKNOWN client's first line for  *
 *     an HTTP request, and if it matches, accumulates the request headers   *
 *     until lc_lws_adopt() can hand the fd - and those already-read bytes - *
 *     to lws via lws_adopt_socket_readbuf().                                *
 *  2. lc_lws_callback(), the lws protocol callback: a plain HTTP GET is     *
 *     served the vendored gamja web IRC client via a mount at "/" (see      *
 *     lc_lws_gamja_mount below); on a completed WS upgrade,                 *
 *     lc_lws_attach_lclient() creates a normal struct lclient and           *
 *     wires it directly into chaosircd's own client/server message         *
 *     dispatch - unlike the standalone tools/ws-irc-gateway.js gateway (a   *
 *     separate QuickJS process relaying to chaosircd over a second, onward  *
 *     plain-TCP connection), there's no onward socket here at all: received *
 *     WS frames are decoded straight into lclient_parse() calls, and        *
 *     outgoing lines are WS-framed via a lclient_vsend() hook instead of    *
 *     chaosircd's normal raw io_write().                                    *
 *  3. Event loop integration: built with -DLWS_WITH_EXTERNAL_POLL:BOOL=ON   *
 *     (see CMakeModules/BuildLibwebsockets.cmake), so lws never runs its    *
 *     own poll() loop - the ADD_POLL_FD/DEL_POLL_FD/CHANGE_MODE_POLL_FD     *
 *     cases in lc_lws_callback() (delivered to protocol 0, i.e. this same   *
 *     callback) register/unregister each lws-owned fd with io_register(),   *
 *     under lc_lws_io_cb_read()/lc_lws_io_cb_write(), which each call       *
 *     lws_service_fd() for that one                                         *
 *     fd; LOCK_POLL/UNLOCK_POLL are no-ops (chaosircd is single-threaded).  *
 *                                                                            *
 * Known limitation: a WS-adopted connection's fd is handed to lws as a       *
 * plain socket. A client connecting through one of chaosircd's own SSL      *
 * listeners (e.g. +6697) and then negotiating WS *inside* that TLS session   *
 * is not supported yet - lws would end up reading/writing raw bytes on a    *
 * fd that's actually still TLS-wrapped below it. For now this only works    *
 * adopting from a plain, non-SSL listen{} port (lws can terminate its own   *
 * TLS for a wss:// listener if that's ever wanted instead).                *
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/hook.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/net.h"
#include "libchaos/str.h"
#include "libchaos/timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/class.h"
#include "ircd/ircd.h"
#include "ircd/lclient.h"

/* -------------------------------------------------------------------------- *
 * libwebsockets                                                              *
 * -------------------------------------------------------------------------- */
#include <libwebsockets.h>

/**
 * @brief io_register() callbacks: forward one ready libchaos fd event into lws.
 *
 * Registered separately per direction (IO_CB_READ -> lc_lws_io_cb_read,
 * IO_CB_WRITE -> lc_lws_io_cb_write) by the ADD_POLL_FD/CHANGE_MODE_POLL_FD
 * cases in lc_lws_callback(), since io_register()'s callback signature
 * carries no way to tell which direction fired. Reporting only the bit that
 * actually happened matters here, not just correctness: some of lws's own
 * internal fds (e.g. its cross-thread cancel-service pipe) are level-
 * triggered ready on one direction essentially all the time, and claiming
 * the other direction is *also* always ready made lws_service_fd() re-arm
 * that same busywork on every call, starving every other fd (including
 * real client sockets) of ever being serviced.
 *
 * @param fd  file descriptor libchaos reports as ready
 * @param arg struct lws_context* this fd belongs to (the io_register() arg)
 */
static void lc_lws_io_cb_read(int fd, void *arg) {
  struct lws_pollfd pfd = {.fd = fd, .events = POLLIN, .revents = POLLIN};

  lws_service_fd((struct lws_context *)arg, &pfd);
}

static void lc_lws_io_cb_write(int fd, void *arg) {
  struct lws_pollfd pfd = {.fd = fd, .events = POLLOUT, .revents = POLLOUT};

  lws_service_fd((struct lws_context *)arg, &pfd);
}

/* -------------------------------------------------------------------------- *
 * A plain HTTP GET on this listener is served gamja (https://codeberg.org/  *
 * emersion/gamja) directly, via this mount at "/" - a tiny web IRC client   *
 * vendored + fetched by CMakeLists.txt's BUILD_LC_LWS block into            *
 * LC_LWS_GAMJA_DIR (installed to <datadir>/gamja). It speaks IRC-over-WS    *
 * directly (no relay/bouncer backend needed, unlike Kiwi IRC's hosted       *
 * client), defaulting to connecting back to "/socket" on whatever host      *
 * served it - which this gateway answers regardless of path, so no         *
 * per-request config generation is needed here at all.                     *
 * -------------------------------------------------------------------------- */
#ifndef LC_LWS_GAMJA_DIR
#error "LC_LWS_GAMJA_DIR must be defined at compile time (see modules/CMakeLists.txt)"
#endif

static const struct lws_http_mount lc_lws_gamja_mount = {
    .mountpoint = "/",
    .mountpoint_len = 1,
    .origin = LC_LWS_GAMJA_DIR,
    .origin_protocol = LWSMPRO_FILE,
    .def = "index.html",
};

/* -------------------------------------------------------------------------- *
 * Per-wsi session data. lws allocates this itself, zeroed (sized via the    *
 * protocol table's per_session_data_size below) before the first callback   *
 * for a given wsi and frees it after the last - this is the anchor the      *
 * protocol callback uses to find "which lclient does this wsi belong to",   *
 * and, via lcptr->plugdata[LCLIENT_PLUGDATA_LWS_SESSION], the reverse:      *
 * which wsi an lclient's outgoing lines (lc_lws_send_hook(), below) should  *
 * be WS-framed onto. A separate slot from LCLIENT_PLUGDATA_LWS (which only  *
 * ever holds a struct lc_lws_sniff*, during bootstrap) - lclient_parse() is *
 * called directly on an already-attached client's lines too (see           *
 * lc_lws_deliver()), which would otherwise make lc_lws_parse_hook           *
 * misinterpret this session as a sniff. rxbuf/rxlen reassemble WS message   *
 * fragments and split them back into '\n'-terminated IRC lines, mirroring   *
 * what io_gets()'s recvq line queue already does for a plain stream socket. *
 * -------------------------------------------------------------------------- */
struct lc_lws_session {
  struct lclient *lcptr; /* set once the WS upgrade has been wired up to an lclient - NULL until then */
  struct lws     *wsi;
  char            rxbuf[IRCD_BUFSIZE];
  size_t          rxlen;
};

/* -------------------------------------------------------------------------- *
 * The lws context lc_lws_adopt() hands sockets over to, and the ADD_POLL_FD  *
 * trampoline registers fds against. Created in lc_lws_load(), destroyed in   *
 * lc_lws_unload(); NULL in between only if creation itself failed, in which  *
 * case lc_lws_parse_hook() no-ops rather than sniffing for a socket it has   *
 * nowhere to hand off to.                                                   *
 * -------------------------------------------------------------------------- */
static struct lws_context *lc_lws_context = NULL;

/* -------------------------------------------------------------------------- *
 * Our vhost, implicitly created by lws_create_context() (we don't set        *
 * LWS_SERVER_OPTION_EXPLICIT_VHOSTS) from the same info as the context, and  *
 * always named "default" when info.vhost_name is left unset (see            *
 * lws_create_vhost() in libwebsockets). lc_lws_adopt() must target this      *
 * vhost explicitly via lws_adopt_socket_vhost_readbuf() rather than the      *
 * context-level lws_adopt_socket_readbuf(): the latter picks a vhost via     *
 * lws_select_vhost(context, -1, ...), which - since we're                   *
 * CONTEXT_PORT_NO_LISTEN (port -1) - is ambiguous with lws's own internal    *
 * "system" vhost (also port -1, created for its own housekeeping), and in   *
 * practice resolves to that one instead of ours, silently routing every     *
 * adopted connection to the wrong (no-op) protocol callback.                *
 * -------------------------------------------------------------------------- */
static struct lws_vhost *lc_lws_vhost = NULL;

/* -------------------------------------------------------------------------- *
 * Class an adopted WS connection's lclient is created under. No listen{}    *
 * block backs a WS-adopted client (the one it originally connected through  *
 * belonged to the discarded bootstrap lclient, see lc_lws_adopt()), so      *
 * there's no config-driven class to inherit - this is a placeholder value.  *
 * -------------------------------------------------------------------------- */
#define LC_LWS_CLASS_NAME "clients"

/* -------------------------------------------------------------------------- *
 * Break the mutual session<->lclient link, if any. Whichever side notices   *
 * the connection is gone first (the wsi closing vs. the lclient exiting)    *
 * calls this so the other side's teardown finds nothing left to double-     *
 * free or write to.                                                        *
 * -------------------------------------------------------------------------- */
static void lc_lws_detach(struct lc_lws_session *session) {
  if (session->lcptr) {
    session->lcptr->plugdata[LCLIENT_PLUGDATA_LWS_SESSION] = NULL;
    session->lcptr = NULL;
  }
}

/**
 * @brief Split reassembled WS message bytes into IRC lines and feed each into lclient_parse().
 *
 * Consumes complete, '\n'-terminated lines first (trimming a preceding
 * '\r'), leaving any trailing partial line in session->rxbuf for the next
 * message to complete - so this can run after every LWS_CALLBACK_RECEIVE
 * regardless of whether it landed on a line boundary or a WS message
 * boundary. But plenty of real IRC-over-WS clients (wscat, Kiwi, ...) send
 * one command per WS text message with no embedded CRLF/LF at all - the
 * message boundary itself IS the line boundary for them - so if @p flush is
 * set (the just-received frame completed a WS message, per
 * lws_is_final_fragment()) and something's still sitting unterminated in
 * rxbuf afterwards, treat that as a complete line too rather than holding
 * it forever waiting for a '\n' that's never coming.
 *
 * @param session the wsi's session data; session->rxbuf/rxlen hold the bytes to process
 * @param flush   1 if the WS message that just added to rxbuf is now complete
 */
static void lc_lws_deliver(struct lc_lws_session *session, int flush) {
  char *p = session->rxbuf;
  char *end = p + session->rxlen;
  char *nl;

  while ((nl = memchr(p, '\n', end - p))) {
    size_t linelen = nl - p;

    if (linelen && p[linelen - 1] == '\r')
      linelen--;

    p[linelen] = '\0';

    if (linelen && session->lcptr)
      lclient_parse(session->lcptr, p, linelen);

    p = nl + 1;
  }

  /* Shift any unterminated remainder to the front for next time. */
  session->rxlen = end - p;
  if (session->rxlen)
    memmove(session->rxbuf, p, session->rxlen);

  if (flush && session->rxlen && session->lcptr) {
    session->rxbuf[session->rxlen] = '\0';
    lclient_parse(session->lcptr, session->rxbuf, session->rxlen);
    session->rxlen = 0;
  }
}

/**
 * @brief lclient_vsend hook: WS-frame a WS-adopted client's outgoing line instead of raw-writing it.
 *
 * Registered at HOOK_DEFAULT on lclient_vsend(), which - after this change -
 * calls it right before its own raw io_write(). Returning 1 there skips that
 * raw write; every other (non-WS) client has no plugdata here, so this
 * returns 0 immediately and lclient_vsend() writes as it always did.
 *
 * @param lcptr client the line is being sent to
 * @param buf   formatted line, CRLF-terminated but not NUL-terminated
 * @param n     length of @p buf
 * @return      1 if handled here (WS-framed), 0 to fall back to the normal raw write
 */
static int lc_lws_send_hook(struct lclient *lcptr, char *buf, size_t n) {
  struct lc_lws_session *session = lcptr->plugdata[LCLIENT_PLUGDATA_LWS_SESSION];
  unsigned char out[LWS_PRE + IRCD_LINELEN + 1];

  if (!session || !session->wsi)
    return 0;

  if (n > IRCD_LINELEN + 1)
    n = IRCD_LINELEN + 1; /* can't happen - lclient_vsend()'s buf is this size - just guard the copy */

  memcpy(out + LWS_PRE, buf, n);

  lws_write(session->wsi, out + LWS_PRE, n, LWS_WRITE_TEXT);

  return 1;
}

/**
 * @brief lclient_release hook: stop a WS-adopted client's fd from being closed twice.
 *
 * lclient_release() (which calls this, at HOOK_DEFAULT, before it touches
 * fds[] itself) unconditionally io_destroy()s whatever's left in fds[] -
 * fine for a normal client, but a WS-adopted one has its real, lws-owned fd
 * sitting there. Blank it out here (same trick lc_lws_adopt() uses on the
 * bootstrap client) so that cleanup skips it, and ask lws to close the wsi
 * (and, with it, the fd) on its own terms instead.
 *
 * @param lcptr the client being released (about to be deleted)
 * @return      always 0 - this never claims to be the client's *only* handler
 */
static int lc_lws_release_hook(struct lclient *lcptr) {
  struct lc_lws_session *session = lcptr->plugdata[LCLIENT_PLUGDATA_LWS_SESSION];

  if (!session)
    return 0;

  lc_lws_detach(session);

  lcptr->fds[0] = -1;
  lcptr->fds[1] = -1;

  if (session->wsi)
    lws_set_timeout(session->wsi, PENDING_TIMEOUT_KILLED_BY_PARENT, LWS_TO_KILL_ASYNC);

  return 0;
}

/**
 * @brief Wire a freshly WS-upgraded wsi into chaosircd's own client/server message dispatch.
 *
 * Creates a normal struct lclient - the same kind lclient_accept() would -
 * except nothing is io_register()ed for its fd, since lws, not libchaos,
 * owns that fd's I/O from here on: reads arrive via LWS_CALLBACK_RECEIVE
 * (see lc_lws_callback()) and lclient_parse() is called directly; writes go
 * out via lc_lws_send_hook() instead of the normal raw io_write().
 *
 * @param wsi     the wsi whose WS upgrade just completed (LWS_CALLBACK_ESTABLISHED)
 * @param session this wsi's per-session data, to be linked to the new lclient
 */
static void lc_lws_attach_lclient(struct lws *wsi, struct lc_lws_session *session) {
  struct lclient *lcptr;
  struct class   *clptr;
  int             fd = lws_get_socket_fd(wsi);
  net_addr_t      addr = 0;
  net_port_t      port = 0;

  if (!io_valid(fd))
    return;

  if (!(clptr = class_find_name(LC_LWS_CLASS_NAME))) {
    log(lclient_log, L_warning, "lc_lws: class '%s' not found, rejecting websocket client",
        LC_LWS_CLASS_NAME);
    lws_set_timeout(wsi, PENDING_TIMEOUT_KILLED_BY_PARENT, LWS_TO_KILL_ASYNC);
    return;
  }

  net_getpeername(fd, &addr, &port);

  if (!(lcptr = lclient_new(fd, addr, port))) {
    log(lclient_log, L_warning, "lc_lws: could not allocate lclient for websocket client");
    lws_set_timeout(wsi, PENDING_TIMEOUT_KILLED_BY_PARENT, LWS_TO_KILL_ASYNC);
    return;
  }

  lcptr->class = class_pop(clptr);

  lcptr->ptimer = timer_start(lclient_exit, lcptr->class->ping_freq, lcptr, "timeout: %llumsecs",
                              lcptr->class->ping_freq);

  timer_note(lcptr->ptimer, "ping timer for %s:%u", net_ntoa(lcptr->addr_remote), lcptr->port_remote);

  session->lcptr = lcptr;
  session->wsi = wsi;
  lcptr->plugdata[LCLIENT_PLUGDATA_LWS_SESSION] = session;

  io_note(lcptr->fds[0], "websocket client from %s:%u", net_ntoa(lcptr->addr_remote),
          (uint32_t)lcptr->port_remote);

  log(lclient_log, L_verbose, "lc_lws: attached websocket client %s:%u", net_ntoa(lcptr->addr_remote),
      lcptr->port_remote);
}

/* -------------------------------------------------------------------------- *
 * Protocol callback. Plain HTTP GETs are served gamja via lc_lws_gamja_mount*
 * (mounted on this vhost, never reaching this callback at all); wires up a  *
 * completed WS upgrade via lc_lws_attach_lclient(); feeds received WS       *
 * frames through lc_lws_deliver() into the attached lclient; tears the     *
 * lclient down when the wsi closes.                                        *
 * -------------------------------------------------------------------------- */
static int lc_lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
  struct lc_lws_session *session = user;

  switch (reason) {
    /* Any plain HTTP GET that lc_lws_gamja_mount (mounted at "/") can
     * serve is handled entirely by lws's own mount code and never reaches
     * this callback - lws only invokes LWS_CALLBACK_HTTP here for a
     * request with no mount hit (e.g. gamja's own fetch("./config.json"),
     * which is optional and absent from the vendored tree). Without a
     * reply here such a request would just hang forever (no 404, nothing)
     * since nothing else answers it, so gamja's fetch() never resolves
     * and the page stays blank waiting on it - reply with a plain 404. */
    case LWS_CALLBACK_HTTP:
      if (lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL))
        return -1;
      if (lws_http_transaction_completed(wsi))
        return -1;
      return 0;

    case LWS_CALLBACK_ESTABLISHED:
      /* Per-session data is lws_zalloc()'d, so `session` itself is only
       * ever NULL here if the protocol has no per_session_data_size - it
       * does (see lc_lws_protocols below), so this is just future-proofing
       * against that changing. */
      if (session)
        lc_lws_attach_lclient(wsi, session);
      break;

    case LWS_CALLBACK_RECEIVE:
      if (!session || !session->lcptr)
        break;

      if (session->rxlen + len >= sizeof(session->rxbuf)) {
        lclient_exit(session->lcptr, "recvq exceeded (%u bytes)", (unsigned)(session->rxlen + len));
        break;
      }

      memcpy(session->rxbuf + session->rxlen, in, len);
      session->rxlen += len;

      lc_lws_deliver(session, lws_is_final_fragment(wsi));
      break;

    case LWS_CALLBACK_CLOSED: {
      /* Unlike ESTABLISHED/RECEIVE, this genuinely can fire with `session`
       * NULL: a connection that never got far enough to have its wsi's
       * per-session data allocated (e.g. a plain static-file GET served by
       * lc_lws_gamja_mount, which never upgrades to WS at all, or one
       * closed before completing the handshake) still gets a CLOSED
       * callback. */
      struct lclient *lcptr = session ? session->lcptr : NULL;

      if (session)
        lc_lws_detach(session);

      if (lcptr)
        lclient_exit(lcptr, "websocket connection closed");

      break;
    }

    /* ---------------------------------------------------------------------
     * External poll loop integration (-DLWS_WITH_EXTERNAL_POLL:BOOL=ON):
     * lws delivers these to protocol 0 (this callback, since "irc-ws" is
     * index 0) instead of running its own poll() loop. `in` is a
     * struct lws_pollargs* naming the fd and, for ADD/CHANGE_MODE, the
     * event mask lws wants (LWS_POLLIN/LWS_POLLOUT, which are plain
     * POLLIN/POLLOUT on this platform).
     * --------------------------------------------------------------------- */
    case LWS_CALLBACK_ADD_POLL_FD: {
      struct lws_pollargs *pa = in;

      /* Client wsi fds are already tracked (they came from chaosircd's own
       * accept()); lws-internal fds (e.g. its cross-thread notify pipe)
       * aren't - give those an io_list entry first so io_register() below
       * has a valid pollfd slot to write into. */
      if (!io_list[pa->fd].type)
        io_new(pa->fd, FD_PIPE);

      if (pa->events & LWS_POLLIN)
        io_register(pa->fd, IO_CB_READ, lc_lws_io_cb_read, lc_lws_context);
      if (pa->events & LWS_POLLOUT)
        io_register(pa->fd, IO_CB_WRITE, lc_lws_io_cb_write, lc_lws_context);

      break;
    }

    case LWS_CALLBACK_DEL_POLL_FD: {
      struct lws_pollargs *pa = in;

      /* Never io_destroy() here: that would syscall_close() the fd, but
       * lws (not us) owns closing it - it does so itself, on its own
       * schedule, separately from telling us to stop polling it. Just drop
       * it from libchaos's tracking. */
      io_unregister(pa->fd, IO_CB_READ);
      io_unregister(pa->fd, IO_CB_WRITE);
      io_forget(pa->fd);

      break;
    }

    case LWS_CALLBACK_CHANGE_MODE_POLL_FD: {
      struct lws_pollargs *pa = in;

      if (pa->events & LWS_POLLIN)
        io_register(pa->fd, IO_CB_READ, lc_lws_io_cb_read, lc_lws_context);
      else
        io_unregister(pa->fd, IO_CB_READ);

      if (pa->events & LWS_POLLOUT)
        io_register(pa->fd, IO_CB_WRITE, lc_lws_io_cb_write, lc_lws_context);
      else
        io_unregister(pa->fd, IO_CB_WRITE);

      break;
    }

    case LWS_CALLBACK_LOCK_POLL:
    case LWS_CALLBACK_UNLOCK_POLL:
      /* chaosircd is single-threaded - nothing to lock. */
      break;

    default:
      break;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Protocol table. "irc-ws" is the only real protocol and must stay at      *
 * index 0 - a WS upgrade with no Sec-WebSocket-Protocol header binds to    *
 * whichever protocol is at the vhost's default_protocol_index (index 0     *
 * unless configured otherwise), so index 0 has to be the real handler, not *
 * a placeholder (this bit us already in tools/ws-irc-gateway.js).          *
 * -------------------------------------------------------------------------- */
static struct lws_protocols lc_lws_protocols[] = {
    {"irc-ws", lc_lws_callback, sizeof(struct lc_lws_session), 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM,
};

/* -------------------------------------------------------------------------- *
 * Accumulates one still-unregistered client's raw HTTP request (with CRLF   *
 * line terminators restored, since io_gets()/lclient_parse() hand us lines  *
 * with those already stripped) until the blank line that ends the headers, *
 * at which point it's handed to lws_adopt_socket_readbuf() verbatim. Capped *
 * at lws's own "ah rx buf" size (2048 bytes) since lws won't take more than *
 * that as readbuf anyway.                                                  *
 * -------------------------------------------------------------------------- */
#define LC_LWS_SNIFF_BUFSIZE 2048

struct lc_lws_sniff {
  char   buf[LC_LWS_SNIFF_BUFSIZE];
  size_t len;
};

static struct sheap lc_lws_sniff_heap;

/**
 * @brief Check whether a freshly-received, not-yet-classified line is an HTTP request line.
 *
 * Only needs to tell HTTP apart from IRC - lws itself sorts out plain GET vs.
 * a WS upgrade GET once the socket is adopted, by reading the rest of the
 * headers we hand it via readbuf.
 *
 * @param s NUL-terminated line, as delivered to the lclient_parse hook
 * @return  1 if it looks like an HTTP/1.x request line, 0 otherwise
 */
static int lc_lws_looks_like_http(const char *s) { return str_match(s, "GET * HTTP/1.?"); }

/**
 * @brief Hand a sniffed-as-HTTP client's fd off to lws, including its already-read header bytes.
 *
 * From this call on, libchaos no longer owns this fd's I/O - lws does, via
 * the ADD_POLL_FD/lc_lws_io_cb_read()/write() trampoline. The lclient block
 * that bootstrapped the raw socket is discarded here; a fresh one gets
 * created later, in lc_lws_attach_lclient(), once/if the WS upgrade actually
 * completes.
 *
 * @param lcptr the still-LCLIENT_UNKNOWN client whose fd is being adopted
 * @param buf   the accumulated HTTP request, headers and terminating blank line included
 * @param len   length of @p buf
 */
static void lc_lws_adopt(struct lclient *lcptr, const char *buf, size_t len) {
  int fd = lcptr->fds[0];

  if (!io_valid(fd))
    return;

  /* Detach libchaos's own callbacks without closing the fd - lws needs it
   * left open, it's about to adopt the very same fd. */
  io_unregister(fd, IO_CB_READ);
  io_unregister(fd, IO_CB_WRITE);

  /* Turn off libchaos's own queued-read/-write handling for this fd: it was
   * enabled (as for any normal client) back when lclient_accept() set this
   * fd up, and left on, io_handle_fd() keeps calling io_queued_read() on
   * every poll() readiness right alongside our own ADD_POLL_FD-registered
   * callback below - stealing bytes off the socket into io_list[fd].recvq
   * before lws's own recv() ever sees them (leaving lws with a permanent
   * EAGAIN), and once recvq.size is nonzero for a queue nothing here ever
   * drains, io_handle_fd() treats that alone as "data pending" forever,
   * calling our read callback in a tight busy loop with nothing to show
   * lws for it. */
  io_queue_control(fd, 0, 0, 0);

  /* Blank out fds[] first: lclient_delete() -> lclient_release() closes
   * whatever's left in there via io_destroy(), which we must not let happen
   * to an fd lws now owns. */
  lcptr->fds[0] = -1;
  lcptr->fds[1] = -1;

  /* plugdata[LCLIENT_PLUGDATA_LWS] here still points at the caller's
   * struct lc_lws_sniff (freed separately by lc_lws_parse_hook right after
   * this returns), not a struct lc_lws_session - clear it before
   * lclient_delete() below fires lclient_release()'s hooks, or
   * lc_lws_release_hook() would misinterpret it as one. */
  lcptr->plugdata[LCLIENT_PLUGDATA_LWS] = NULL;

  lws_adopt_socket_vhost_readbuf(lc_lws_vhost, fd, buf, len);

  lclient_delete(lcptr);
}

/**
 * @brief lclient_parse hook: sniff for HTTP on unregistered clients and adopt matches into lws.
 *
 * Registered at HOOK_DEFAULT on lclient_parse(), which already fires with
 * every raw, untokenized line before anything assumes it's IRC - the same
 * plug-in point lc_mflood.c uses. Returning 1 swallows the line (chaosircd's
 * own IRC parsing never sees it); once a client stops looking relevant here
 * (already past LCLIENT_UNKNOWN, or its first line wasn't an HTTP request
 * line) this returns 0 immediately and never touches it again.
 *
 * @param lcptr the client this line came from
 * @param s     NUL-terminated line, CRLF already stripped by io_gets()
 * @return      1 to swallow the line (handled here or accumulating), 0 to let normal IRC parsing run
 */
static int lc_lws_parse_hook(struct lclient *lcptr, char *s) {
  struct lc_lws_sniff *sniff;
  size_t                n;

  if (!lc_lws_context)
    return 0;

  /* io_gets() only strips the trailing CRLF/LF when built with -DDEBUG (see
   * the #ifdef DEBUG block in lclient_process(), src/lclient.c) - otherwise
   * lclient_parse() (and this hook, firing at its very top) sees it still
   * attached. Trim it ourselves so matching/accumulation below don't care
   * either way, and so the CRLF we re-append per line isn't doubled up. */
  n = str_len(s);
  while (n && (s[n - 1] == '\r' || s[n - 1] == '\n'))
    s[--n] = '\0';

  sniff = lcptr->plugdata[LCLIENT_PLUGDATA_LWS];

  if (!sniff) {
    if (!lclient_is_unknown(lcptr) || !lc_lws_looks_like_http(s))
      return 0;

    if (!(sniff = mem_static_alloc(&lc_lws_sniff_heap)))
      return 0;

    sniff->len = 0;
    lcptr->plugdata[LCLIENT_PLUGDATA_LWS] = sniff;
  }

  if (sniff->len + n + 2 > sizeof(sniff->buf)) {
    /* Request headers too large for what we can hand to lws - give up and
     * let the connection die the normal way (as unrecognised garbage). */
    mem_static_free(&lc_lws_sniff_heap, sniff);
    lcptr->plugdata[LCLIENT_PLUGDATA_LWS] = NULL;
    return 0;
  }

  memcpy(sniff->buf + sniff->len, s, n);
  sniff->len += n;
  sniff->buf[sniff->len++] = '\r';
  sniff->buf[sniff->len++] = '\n';

  if (n == 0) {
    /* Blank line: end of HTTP headers - hand off to lws now. */
    lc_lws_adopt(lcptr, sniff->buf, sniff->len);
    mem_static_free(&lc_lws_sniff_heap, sniff);
  }

  return 1;
}

/**
 * @brief Forward libwebsockets' own internal logging into chaosircd's log().
 *
 * lws has its own independent lwsl_*()-based logging, entirely separate
 * from chaosircd's log()/debug() - without this it's invisible even at
 * chaosircd's "debug" log level. Only errors/warnings by default; the
 * lower levels (LLL_NOTICE/LLL_INFO/LLL_DEBUG) are noisy enough to only be
 * worth enabling here temporarily while diagnosing something.
 */
static void lc_lws_log_emit(int level, const char *line) {
  (void)level;
  log(lclient_log, L_status, "lc_lws lws: %s", line);
}

/**
 * @brief Create the lws context lc_lws_adopt() and the poll trampoline use.
 *
 * No listening socket of its own: sockets only ever arrive already-accepted,
 * via lws_adopt_socket_vhost_readbuf() in lc_lws_adopt(), onto the implicit
 * default vhost this creates. Uses CONTEXT_PORT_NO_LISTEN_SERVER (-2), not
 * CONTEXT_PORT_NO_LISTEN (-1): the latter is also what lws hardcodes for its
 * own internal "system" vhost (see lws_create_context() in context.c), and
 * server.c's Host-header vhost rebind (lws_select_vhost(), keyed on
 * wsi->a.vhost->listen_port when nonzero) would then ambiguously match
 * either vhost by port alone and could rebind an adopted wsi onto "system"
 * instead of ours, silently routing it to no protocol callback of ours.
 *
 * @return 0 on success, -1 if lws_create_context() failed
 */
static int lc_lws_context_create(void) {
  struct lws_context_creation_info info;

  lws_set_log_level(LLL_ERR | LLL_WARN, lc_lws_log_emit);

  memset(&info, 0, sizeof(info));

  info.port = CONTEXT_PORT_NO_LISTEN_SERVER;
  info.protocols = lc_lws_protocols;
  info.mounts = &lc_lws_gamja_mount;
  info.gid = -1;
  info.uid = -1;

  if (!(lc_lws_context = lws_create_context(&info))) {
    log(lclient_log, L_warning, "lc_lws: lws_create_context() failed");
    return -1;
  }

  if (!(lc_lws_vhost = lws_get_vhost_by_name(lc_lws_context, "default"))) {
    log(lclient_log, L_warning, "lc_lws: could not find our own implicit \"default\" vhost");
    lws_context_destroy(lc_lws_context);
    lc_lws_context = NULL;
    return -1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_lws_load(void) {
  mem_static_create(&lc_lws_sniff_heap, sizeof(struct lc_lws_sniff), LCLIENT_BLOCK_SIZE / 2);
  mem_static_note(&lc_lws_sniff_heap, "lc_lws HTTP sniff heap");

  if (lc_lws_context_create()) {
    mem_static_destroy(&lc_lws_sniff_heap);
    return -1;
  }

  hook_register(lclient_parse, HOOK_DEFAULT, lc_lws_parse_hook);
  hook_register(lclient_vsend, HOOK_DEFAULT, lc_lws_send_hook);
  hook_register(lclient_release, HOOK_DEFAULT, lc_lws_release_hook);

  log(lclient_log, L_status, "lc_lws: loaded");

  return 0;
}

void lc_lws_unload(void) {
  hook_unregister(lclient_parse, HOOK_DEFAULT, lc_lws_parse_hook);
  hook_unregister(lclient_vsend, HOOK_DEFAULT, lc_lws_send_hook);
  hook_unregister(lclient_release, HOOK_DEFAULT, lc_lws_release_hook);

  if (lc_lws_context) {
    lws_context_destroy(lc_lws_context);
    lc_lws_context = NULL;
  }

  mem_static_destroy(&lc_lws_sniff_heap);

  log(lclient_log, L_status, "lc_lws: unloaded");
}
