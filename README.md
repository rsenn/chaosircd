# chaosircd

*peace, love & hippie edition*

chaosircd is an IRC daemon built around runtime-(re)loadable modules —
every client command, channel mode, user mode, and flood-control check is
its own `.so`, loaded and swapped without restarting the server. DNS,
ident (RFC 1413), and proxy-scanning all run in a single dedicated child
process, so a flood of connecting clients doesn't cost the main event loop
a file descriptor or a blocked `gethostbyname()` each. Server links use
timestamps for netsplit handling, in the same family as TS5/hybrid, and
both server-server and client-server connections can run over OpenSSL.

## Why it exists

chaosircd was conceived as a from-scratch IRC daemon, written to keep a
couple of real networks running: the IRC services behind
`irc.blah.ch` at freemails.ch, and a handful of servers on a home
cablemodem with a dynamic IP behind a dynamic-DNS hostname. Hybrid's
server-linking wanted static addresses; a cablemodem doesn't have one, so
the first real feature was teaching server links to resolve a hostname at
connect time instead of requiring a bare IP. `/SPOOF` came soon after, so
clients on that link could hide their real address the same way the
servers already did.

From there the scope kept growing: a self-contained DNS resolver
(`lib/servauth/dns.c`, in the classic djb two-file style), an open-proxy
scanner, and an ident (port 113) client, all so a connecting client could
be checked without trusting whatever it claimed about itself. K-lines and
G-lines don't just refuse a banned client at the application layer —
they hand the address to a BPF socket filter on Linux, so a client that's
already been told to go away stops costing the daemon a `read()` at all.

Implementation borrowed the `dlink` doubly-linked-list code from
[ircd-hybrid](https://ircd-hybrid.org/) — you can still see the family
resemblance in `lib/src/dlink.c` — but everything built on top of it was
written from scratch as its own daemon from the start.

That was 2003. The project has had long quiet stretches since — the git
history jumps from an initial 2007 import to 2014 to 2026 — but each time
it's come back to run something real, most recently a
[WebSocket gateway](#the-websocket-gateway) so a browser can join a
chaosircd network with no separate bouncer process, and the start of an
[IRCv3 migration](IRCv3.md) so modern clients (gamja, Kiwi, weechat's
relay protocol, and friends) work with it properly instead of just well
enough.

## Features

- **Loadable modules for everything.** Client commands, channel modes,
  user modes, and flood control (`modules/msg`, `modules/chanmode`,
  `modules/usermode`) are independent `.so` files — currently around 90 of
  them — loaded per `modules.conf` and swappable without a restart.
- **A dedicated servauth process.** `lib/servauth/` runs DNS resolution,
  ident lookups, and proxy scanning for every connecting client in one
  child process (`dns.c`, `auth.c`, `proxy.c`), so the main daemon never
  blocks on any of the three.
- **Hostname-based server links**, for servers that don't have a static
  address to link to in the first place.
- **`/SPOOF`**, so a client's real address doesn't have to be their
  visible one.
- **Socket-filter-backed K/G-lines.** Bans push a BPF program onto the
  listening socket on Linux (`lib/src/filter.c`), so a banned address is
  refused before it costs the daemon a read.
- **Timestamp-based netsplit handling** for server-server links, in the
  same TS5/hybrid family, over plaintext or OpenSSL.
- **A WebSocket gateway (`lc_lws`)**, self-hosting the
  [gamja](https://codeberg.org/emersion/gamja) web client — see below.
- **An [IRCv3 migration](IRCv3.md) underway** — real `CAP` negotiation
  and `echo-message` landed first; the rest is planned out phase by phase.

### The WebSocket gateway

`modules/lclient/lc_lws.c` is an in-process WebSocket↔IRC gateway: a
browser can speak IRC directly over `ws://`/`wss://` to the same listener
a normal client would use, no separate bouncer or relay process in
between. It's built on [libwebsockets](https://libwebsockets.org/),
driven from chaosircd's own event loop rather than running a second one,
and doubles as a static file server for a vendored, unmodified copy of
[gamja](https://codeberg.org/emersion/gamja) — a small IRC web client
that needs no build step (no npm/yarn touches this repository; see
[`CLAUDE.md`](CLAUDE.md)). It's off by default, since it pulls in
libwebsockets as a dependency nothing else here needs:

```sh
cmake -S . -B build -DBUILD_LC_LWS=ON
cmake --build build
```

## Building

chaosircd builds with CMake:

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
sudo cmake --install build
```

See [`INSTALL`](INSTALL) for OpenSSL certificate generation and the
BPF/socket-filter prerequisites for K/G-lines. Building loadable modules
requires per-module shared-library support, on by default everywhere but
Windows/Cygwin (`BUILD_LIBIRCD`, `CMakeLists.txt`).

## Status

Actively maintained, in long bursts separated by long quiet ones — this is
primarily a project that gets worked on when it's running something for
real. The [IRCv3 migration plan](IRCv3.md) is the current roadmap; that
document also doubles as a running list of the non-obvious lessons this
codebase has taught back to whoever's touching it next.

## License

The core library (`lib/`) is LGPL v2 or later; the daemon and its modules
(`src/`, `modules/`) are GPL v2 or later — see the header of any given
file for the exact terms, and [`COPYING`](COPYING) /
[`COPYING.LESSER`](COPYING.LESSER) for the full license texts.
