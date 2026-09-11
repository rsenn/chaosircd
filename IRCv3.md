# chaosircd IRCv3 Migration Plan

chaosircd predates IRCv3 (its `CAP` handler was a permanently-empty stub:
every `CAP LS` got back `CAP * LS :` regardless of subcommand, and nothing
was ever tracked per-client). This document is the plan for bringing it up
to a useful subset of IRCv3, in the order it should be done.

## Ground rules established while implementing step 1

These aren't optional style points - they're bugs this migration already
hit once each, so every later step should assume them:

- **Registration doesn't wait for `CAP END`.** chaosircd completes
  registration as soon as `NICK`+`USER` are both in, regardless of whether
  the client ever sent `CAP` at all. This is a deliberate non-change (much
  bigger, riskier surgery to the registration state machine for little
  benefit) - it just means every `CAP` subcommand handler has to work
  correctly whether it fires before, after, or interleaved with
  `NICK`/`USER`, not assume a fixed order.
- **`struct client *cptr` can be `NULL` in a `MFLG_UNREG` handler.**
  `struct client` is only allocated once registration completes;
  `struct lclient` exists from the moment the connection is accepted. Any
  per-connection state a `CAP`-negotiated capability needs to track has to
  live on `struct lclient`, and must be reachable (and usable) before
  `cptr` exists - real clients routinely send `CAP REQ` before `NICK`/`USER`.
  (Caught this the hard way: `mr_cap()` dereferencing `cptr->name` for a
  pre-registration `CAP REQ` segfaulted the whole daemon.)
- **Never add a field in the middle of `struct client` or `struct lclient`.**
  Both are shared, fixed-layout structs that every loadable module (`.so`)
  was compiled against independently. Inserting a field shifts the offset
  of everything declared after it - any module not rebuilt in lockstep
  silently reads/writes the wrong bytes at runtime (no compiler error, no
  link error, just corrupted state or a segfault under load). Both structs
  already have a `void *plugdata[32]` array at the very end for exactly
  this reason (see `lc_lws`'s use of `LCLIENT_PLUGDATA_LWS_SESSION` and
  this migration's `LCLIENT_PLUGDATA_CLICAPS`) - a new capability's state
  goes in a plugdata slot (or, if it doesn't fit in a pointer, a
  heap-allocated struct pointed to by one), never a new named field, unless
  every single loadable module is being rebuilt and reinstalled in the same
  step.
- **The channel broadcast fast path bypasses per-connection send hooks.**
  `channel_vsend()`'s multicast path (`io_multi_link()`) links a
  pre-formatted buffer straight into a recipient's fd sendq, bypassing
  `lclient_vsend()` entirely for performance (see `lclient_send_raw()` and
  its one caller in `channel.c` for the escape hatch this migration added).
  Anything IRCv3-related that needs per-recipient framing or content (tag
  filtering per capability, for instance - see message-tags below) has to
  either go through `lclient_vsend()`/`lclient_send_raw()` instead of
  `channel_send()`, or `channel_vsend()` needs to grow real per-recipient
  variation instead of one shared buffer. Worth deciding explicitly in the
  message-tags step, not discovering by accident.

## Where things live

- `modules/msg/m_cap.c` - the `CAP` command handler and the capability
  table (`m_cap_table[]`). Add a row here plus real behaviour elsewhere to
  support a new capability; the table is what answers `LS`/`REQ`/`LIST`.
- `include/ircd/lclient.h` - `CLICAP_*` bit constants, the
  `LCLIENT_PLUGDATA_CLICAPS` slot, and the `lclient_clicaps()` /
  `lclient_set_clicaps()` accessors. Check a capability with
  `lclient_clicaps(lcptr) & CLICAP_WHATEVER`.
- Message-sending core (`src/channel.c`, `src/client.c`, `src/lclient.c`)
  - where a capability actually changes server behaviour (echoing,
  tagging, batching, ...).

## Phased plan

Ordered by (mostly) increasing effort/risk, not spec numbering. Do a phase
fully - implementation, a rebuild+reinstall, and a wscat/gamja round-trip
test - before starting the next; several later phases assume earlier ones
are solid (message-tags in particular is load-bearing for most of the
back half of this list).

### Phase 1 - `echo-message` - **DONE**

The server echoes a client's own `PRIVMSG`/`NOTICE` (channel or user
target) back to the sending connection when this cap is enabled, instead
of the client having to fake it locally. Implemented in
`channel_message()` (`src/channel.c`) and `client_message()`
(`src/client.c`), gated on `CLICAP_ECHO_MESSAGE`.

Picked first because: it was the concrete bug in front of us (gamja
double-displaying self-sent messages because chaosircd never granted this
cap, so gamja fell back to unconditional local echo), it's genuinely
simple (a handful of extra `client_send()` calls, no new wire concepts),
and it forced building the *real* `CAP LS`/`REQ`/`ACK`/`NAK`/`LIST`
negotiation machinery (`m_cap.c`) that every later phase reuses. Nothing
after this needs a second implementation of CAP negotiation itself - just
a new row in `m_cap_table[]` and a new `CLICAP_*` bit.

### Phase 2 - `cap-notify`

Tiny. When a capability an already-registered client didn't negotiate
becomes available or unavailable at runtime (module load/unload is the
only realistic trigger here - chaosircd doesn't have config reload of
individual caps), send unsolicited `CAP <nick> NEW :<caps>` /
`CAP <nick> DEL :<caps>` to clients that negotiated `cap-notify` itself.
Given chaosircd's capability set is static per running process (nothing
today changes `m_cap_table[]` at runtime), this is close to a no-op
initially - implement the plumbing (a small hook other module load/unload
paths can call) but it won't fire in practice until some future capability
*is* conditional on e.g. a module being loaded. Cheap to do now while
`m_cap.c` is fresh in mind; low value until something needs it.

### Phase 3 - message-tags

The foundational one - `server-time`, `batch`, `labeled-response`,
`account-tag`, `msgid`, and draft `chathistory` all sit on top of this.
Scope:

- Parse an optional leading `@tag1=val;tag2 ` block off incoming client
  lines before normal command parsing (the IRCv3 client-tags a server
  should ever accept from a client are few - mostly just `+draft/reply`
  and similar - reject/strip anything else per spec rather than trust
  arbitrary client-supplied tags).
- Emit tags on outgoing lines *conditionally per recipient* - a client that
  didn't negotiate `message-tags` must get the untagged line. This is
  exactly the per-recipient-variation problem flagged in "ground rules"
  above: `channel_send()`'s shared-buffer fast path can't vary its output
  per recipient, so tagged output has to bypass it (`lclient_send_raw()`
  per WS-adopted-client precedent, or a second parallel buffer keyed by
  whether the recipient negotiated `message-tags`).
- `IRCD_LINELEN` is already 2048 (this server was never strictly
  512-byte-limited), but the *tag* budget in the spec is separately
  capped (4094 bytes club including the leading `@`, or 8191 for
  `message-tags` alone depending on spec revision) - decide and enforce a
  tag-section cap independent of the existing line cap.

Bigger than phases 1-2, but every field/flag it needs (parser hook point,
per-recipient send path) already exists somewhere in the codebase from
this session's `lc_lws` work or phase 1 - this is integration, not new
infrastructure from scratch.

### Phase 4 - `server-time`

Trivial once phase 3 lands: attach a `time=<rfc3339>` tag to every message
a client with this cap negotiated receives. No new state, no new command.

### Phase 5 - small independent caps (do together, each is an afternoon)

Each of these is a self-contained row in `m_cap_table[]` plus a couple of
lines at one existing call site - no shared infrastructure work beyond
phase 1, safe to batch:

- **`away-notify`** - `modules/msg/m_away.c` already exists (`AWAY`
  command); just broadcast `AWAY`/un-`AWAY` to channel-mates who
  negotiated the cap, same shape as the JOIN-echo fix from this session.
- **`extended-join`** - append account name (or `*`) and realname to the
  `JOIN` line for clients that negotiated it. Blocked on there being a
  concept of "account" at all - chaosircd's `m_userdb.c` is an
  auth/registration lookup via `sauth`, not a NickServ-style persistent
  account; decide what (if anything) populates the account field before
  implementing, or ship `*` (no account) unconditionally until accounts
  exist.
- **`userhost-in-names`** - prefix each nick in `RPL_NAMREPLY` with
  `user@host` instead of just the nick, for negotiating clients. Small
  change to `modules/msg/m_names.c`.
- **`invite-notify`** - broadcast an `INVITE` notice to channel ops who
  negotiated it, alongside the existing single-target `INVITE` in
  `modules/msg/m_invite.c`.
- **`chghost`** - broadcast a `CHGHOST` line to negotiating clients
  whenever a client's displayed `user@host` changes. Check whether
  anything in chaosircd currently *can* change a live client's host/ident
  post-registration (vhost/spoof modules - `m_spoof.c` exists) before
  wiring this up; if nothing does yet, this one has no trigger to hook.

### Phase 6 - `multi-prefix`

Client asks to see *every* status prefix it holds in a channel (e.g.
`@%+nick` for someone who's op+halfop+voice) instead of just the highest.
chaosircd's `PREFIX=(hov)@%+` (seen in `RPL_ISUPPORT`) already models
three prefix levels, so the data is there - `modules/msg/m_names.c` and
`modules/msg/m_who.c` just need to emit all held prefixes instead of the
first match, when negotiated.

### Phase 7 - `setname`

Lets a registered client change its realname (`info` field) without
reconnecting, broadcast as a `SETNAME` line to anyone who can see it (same
delivery shape as `NICK` changes). Needs a new client-issuable command
plus the notify-on-change plumbing; check whether anything server-side
(opers, stats) assumes `info` is immutable post-registration before
allowing it to change live.

### Phase 8 - `batch`

Wraps a group of related lines (typically a channel's worth of
`chathistory` playback, or a netsplit's worth of `QUIT`s) in
`BATCH +ref <type> ...` / `BATCH -ref`, with each wrapped line tagged
`@batch=ref`. Needs phase 3 (tags) done first. On its own this phase adds
no new *content* - it's infrastructure for phase 10's `CHATHISTORY`, and
is also usable later for large `KICK`/`QUIT` fan-out (netsplit batches).
Low priority to build before something actually needs it; grouped here
because it's a prerequisite, not because it's independently valuable yet.

### Phase 9 - `labeled-response`

Client tags an outgoing command with `@label=xyz`; the server tags every
line generated in direct response the same way, wrapped in a `batch` if
there's more than one, so the client can correlate replies without
guessing from context. Needs phases 3 and 8. Medium effort: every message
handler that currently just calls `client_send()`/`numeric_send()`
in response to a command needs the active label (if any) threaded through
and re-attached - a `struct client`/`lclient` "current label" slot set at
dispatch time and consumed by the send path is the shape to aim for,
similar to how `client_source` is set-then-consumed today
(`src/client.c`).

### Phase 10 - `standard-replies` (`FAIL`/`WARN`/`NOTE`)

Structured, machine-parseable alternative to numeric errors
(`FAIL <cmd> <code> [context...] :<description>`). Worth doing once
phases 3 and 9 exist since these replies are most useful labeled, but the
command itself doesn't strictly require either - could move earlier if a
specific error path wants it sooner. Mechanically: a new
`numeric_send()`-shaped helper (`std_reply_send()` or similar) that
command handlers opt into instead of `numeric_send()`, one at a time -
this does **not** need to replace every existing numeric in one pass, and
shouldn't (huge, low-value diff for no behavioural change on its own).

### Phase 11 - SASL (`PLAIN` mechanism first)

The first phase that's a genuinely open design question, not just wiring:
chaosircd has no persistent account system today (`m_userdb.c`/`sauth` is
an auth/proxy-check lookup, not credential storage tied to a nick).
`SASL PLAIN` needs *something* to check a submitted username/password
against. Before writing any `AUTHENTICATE` handling, decide:

- What backs an "account" - a new flat file/INI table (`libchaos/ini.h` is
  already used elsewhere, e.g. class/oper config), or a new small database
  module - and whether it's chaosircd's job at all versus deferring to a
  services package (Atheme/Anope-style) talking a services protocol
  chaosircd doesn't currently implement (`m_nservice.c` exists but scope
  unclear - check before assuming either way).
- Whether SASL replaces or supplements the oper `PASS`/`m_pass.c` flow.

This is the one phase in this plan that's a project of its own, not an
afternoon - don't start it opportunistically the way phases 1-7 can be;
scope it separately once everything before it is done and there's a real
account backend to build on.

### Phase 12 - `CHATHISTORY` (draft, gated on phase 8)

Bouncer-style playback of channel history on join/request. chaosircd has
no message persistence today - this needs a storage layer (what, how much
retained, per-channel or per-user) designed before the command itself is
worth writing. Explicitly out of scope until something concrete asks for
it; listed here only so it's not forgotten as the natural next step once
`batch` exists.

### Adjacent, not core IRCv3 - worth doing alongside this plan

- **`WEBIRC`-style real-client-IP passthrough for `lc_lws`.** Every
  gamja/WS client currently appears to chaosircd as connecting from
  `127.0.0.1` (or wherever the gateway's listener is bound) - fine for a
  single-host setup, wrong the moment `lc_lws` and chaosircd aren't on the
  same box, and wrong for any host-based ban/class matching today. Not an
  IRCv3 capability (no client opts in - it's gateway-to-server trust, like
  real `WEBIRC` or `PROXY` protocol), but directly relevant to the work
  this session did on `lc_lws` and worth scoping whenever that module gets
  revisited.
- **STS (`draft/sts` / RFC: strict transport security).** Tells a client
  "always use TLS to reach this server after this point". chaosircd
  already has SSL contexts and both a plaintext and SSL `listen{}` block
  configured (see `ircd.conf`) - mechanically small once `message-tags`
  or even just a CAP value with a policy string exists (STS is delivered
  as a CAP value, e.g. `CAP * LS :sts=port=6697,duration=2592000`, no tags
  needed). Could move much earlier in this list - low effort - but low
  urgency until TLS is actually the expected default transport.

## Testing pattern for every phase

Every phase in this plan should be verified the same way phase 1 was,
before moving on:

1. `wscat`/raw-socket round trip exercising the new behaviour directly
   (fastest feedback, no browser involved) - see the `script -qec
   'wscat --connect ws://127.0.0.1:7778/ -x "..." ...'` pattern used
   throughout this session for scripted multi-line sequences.
2. A **regression** check with a plain client that negotiates nothing -
   confirm behaviour for clients that never send `CAP` at all is
   byte-for-byte unchanged.
3. Only then, gamja in a real browser for the end-to-end/UX check.

And after any change to `struct client`, `struct lclient`, or any other
struct a loadable module might have its own compiled-in copy of the
layout for: rebuild **every** currently-loaded module, not just the ones
directly touched, or use a plugdata slot instead and rebuild only what
actually changed (see "Ground rules" above - this is not optional, it's
how the phase 1 implementation crashed the daemon the first time round).
