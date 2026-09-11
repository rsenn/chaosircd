#!/bin/sh
#
# common.sh - shared helpers for servauth unit tests
#
# Provides:
#   - sv_start / sv_stop        : launch/kill servauth as a background process
#                                  and talk to it over a pair of named pipes,
#                                  using the same control protocol ircd uses
#                                  over the socketpair
#   - sv_send / sv_expect       : send a command line, wait for a reply
#   - mock_proxy_*              : spawn netcat listeners that speak just
#                                  enough of each proxy protocol to make
#                                  servauth classify them as open/denied/etc
#   - cleanup_mocks             : kill any netcat listeners started by a test
#
# Every test script should `.` (source) this file, call sv_start once, run
# its checks, then call sv_stop (trap on EXIT is recommended, see
# test_*.sh). Written for plain POSIX sh - tested under dash and shish/ash,
# not just bash.

set -u

SV_TIMEOUT=${SV_TIMEOUT:-5}

# ---------------------------------------------------------------------------
# locate the servauth binary
#
# Assumes the caller has already `cd`ed into tests/servauth (test_*.sh does
# this before sourcing common.sh), so paths below are relative to that.
# ---------------------------------------------------------------------------
sv_find_binary() {
  if [ -n "${SERVAUTH_BIN:-}" ] && [ -x "$SERVAUTH_BIN" ]; then
    echo "$SERVAUTH_BIN"
    return 0
  fi

  local c
  for c in \
    "../../build/x86_64-linux-debug/lib/servauth/servauth" \
    "../../build/x86_64-linux-gnu/lib/servauth/servauth" \
    "../../build/x86_64-linux-clang/lib/servauth/servauth" \
    "/usr/local/libexec/servauth"
  do
    if [ -x "$c" ]; then
      echo "$c"
      return 0
    fi
  done

  return 1
}

# ---------------------------------------------------------------------------
# start/stop servauth as a background process, connected via a pair of
# named pipes.
#
# servauth defaults to fd 0 (read) / fd 1 (write) when launched with no
# argv, exactly like `child.c` does for the "socketpair" case - so we can
# just run it as a normal pipe-connected child process, no socketpair(2)
# gymnastics required.
# ---------------------------------------------------------------------------
sv_start() {
  local bin
  bin="$(sv_find_binary)" || {
    echo "sv_start: could not find servauth binary (set SERVAUTH_BIN)" >&2
    return 1
  }

  SV_TMPDIR=$(mktemp -d)
  mkfifo "$SV_TMPDIR/in" "$SV_TMPDIR/out"

  # servauth opens its own fd for each fifo path (via the < and >
  # redirections below), rather than inheriting a dup of a fd we already
  # have open - servauth does its own fcntl(F_SETFL) on stdin/stdout for
  # non-blocking I/O, and a dup would share that file *description* with
  # our fds, silently making our reads/writes non-blocking too. Separate
  # open() calls on the same fifo path get independent descriptions, so
  # servauth's flag changes stay local to its own end.
  #
  # Opening a fifo for one direction blocks until a peer opens the other
  # end, so the two sides must be opened in matching order: servauth's
  # stdin/stdout opens happen as part of backgrounding it below, and the
  # `exec` lines right after unblock each of those opens in turn.
  "$bin" <"$SV_TMPDIR/in" >"$SV_TMPDIR/out" &
  SV_PID=$!

  exec 5>"$SV_TMPDIR/in"
  exec 6<"$SV_TMPDIR/out"

  # give it a moment to finish servauth_init()
  sleep 0.2

  if ! kill -0 "$SV_PID" 2>/dev/null; then
    echo "sv_start: servauth ($bin) exited immediately" >&2
    return 1
  fi

  sv_trace "# servauth started: $bin (pid $SV_PID)"

  # sv_expect's read-with-timeout loop is farmed out to this helper script
  # (see below). It must be written here, at top level, rather than from
  # inside sv_expect itself - some ash-family shells (shish included) mis-
  # handle a heredoc-fed redirect when it runs inside a function that is
  # itself invoked through command substitution, and leak the heredoc body
  # to stdout instead of the file.
  SV_EXPECT_HELPER="$SV_TMPDIR/expect.sh"
  cat >"$SV_EXPECT_HELPER" <<'EOF'
#!/bin/sh
prefix="$1"
while IFS= read -r line; do
  printf 'IN:%s\n' "$line"
  case "$line" in
    "$prefix"|"$prefix "*)
      printf 'MATCH:%s\n' "$line"
      exit 0
      ;;
  esac
done
exit 1
EOF
  chmod +x "$SV_EXPECT_HELPER"

  return 0
}

sv_stop() {
  [ -n "${SV_PID:-}" ] || return 0

  # belt-and-suspenders: also reap any direct child, in case servauth
  # re-execs or forks internally
  local child
  child=$(pgrep -P "$SV_PID" 2>/dev/null)

  kill "$SV_PID" 2>/dev/null
  [ -n "$child" ] && kill "$child" 2>/dev/null

  wait "$SV_PID" 2>/dev/null

  exec 5>&- 2>/dev/null
  exec 6<&- 2>/dev/null
  [ -n "${SV_TMPDIR:-}" ] && rm -rf "$SV_TMPDIR"

  unset SV_PID SV_TMPDIR SV_EXPECT_HELPER
}

# Verbose raw-traffic tracing. On by default (that's the whole point of
# this harness); set SV_VERBOSE=0 to silence it. Goes to stderr so it
# never pollutes `reply=$(sv_expect ...)` command-substitution capture.
SV_VERBOSE=${SV_VERBOSE:-1}

sv_trace() {
  [ "$SV_VERBOSE" = "1" ] || return 0
  echo "$@" >&2
}

# sv_send <command line...>
#
# One line == one command, exactly as sent by ircd's sauth_* functions.
# NOTE: servauth treats an unrecognised or malformed command as fatal and
# shuts itself down - keep commands well-formed (see servauth's
# lib/servauth/commands.c for exact argument counts).
sv_send() {
  sv_trace ">> $*"
  echo "$*" >&5
}

# sv_expect <prefix> [timeout]
#
# Reads lines from servauth until one starts with <prefix> (space-anchored,
# so "dns forward 1" won't match "dns forward 12"), or the timeout elapses.
# Echoes the matching line on success, returns 1 on timeout/EOF. Every
# line read (matching or not) is traced to stderr as it arrives.
#
# POSIX `read` has no -t/-u options (dash/ash don't support them), so the
# read loop is farmed out to a small helper script and bounded with the
# external `timeout` command instead.
sv_expect() {
  local prefix="$1"
  local timeout="${2:-$SV_TIMEOUT}"
  local output status outline

  output=$(timeout "$timeout" "$SV_EXPECT_HELPER" "$prefix" <&6)
  status=$?

  echo "$output" | while IFS= read -r outline; do
    case "$outline" in
      IN:*) sv_trace "<< ${outline#IN:}" ;;
    esac
  done

  if [ "$status" -eq 0 ]; then
    echo "$output" | sed -n 's/^MATCH://p' | tail -n1
    return 0
  fi

  return 1
}

# ---------------------------------------------------------------------------
# proxy mocks
#
# servauth connects out to the address given in the "proxy" command, writes
# a protocol-specific probe, then classifies the reply. These helpers start
# a one-shot netcat listener that plays the server side of that protocol.
# ---------------------------------------------------------------------------
MOCK_PIDS=""

# mock_raw_reply <port> <bytes-file>
#
# Generic helper: listen once on <port>, send the contents of <bytes-file>
# to whoever connects, then exit. Does not attempt to read/validate the
# client's request - servauth only cares about what comes back.
mock_raw_reply() {
  local port="$1"
  local file="$2"

  nc -l -p "$port" -q1 <"$file" >/dev/null 2>&1 &
  MOCK_PIDS="$MOCK_PIDS $!"

  # give the listener a moment to bind before the caller connects
  sleep 0.2
}

# mock_http_open <port>
# Replies like a wide-open HTTP CONNECT proxy.
mock_http_open() {
  local port="$1"
  local tmp
  tmp="$(mktemp)"
  printf 'HTTP/1.0 200 Connection established\r\n\r\n' >"$tmp"
  mock_raw_reply "$port" "$tmp"
}

# mock_http_denied <port>
# Replies with a non-2xx/5xx status - servauth should classify as "denied".
mock_http_denied() {
  local port="$1"
  local tmp
  tmp="$(mktemp)"
  printf 'HTTP/1.0 403 Forbidden\r\n\r\n' >"$tmp"
  mock_raw_reply "$port" "$tmp"
}

# mock_socks4_open <port>
# Replies with SOCKS4 "request granted" (0x5A).
mock_socks4_open() {
  local port="$1"
  local tmp
  tmp="$(mktemp)"
  printf '\x00\x5a\x00\x00\x00\x00\x00\x00' >"$tmp"
  mock_raw_reply "$port" "$tmp"
}

# mock_socks4_denied <port>
# Replies with SOCKS4 "request rejected" (0x5B).
mock_socks4_denied() {
  local port="$1"
  local tmp
  tmp="$(mktemp)"
  printf '\x00\x5b\x00\x00\x00\x00\x00\x00' >"$tmp"
  mock_raw_reply "$port" "$tmp"
}

# mock_socks5_open <port>
# Replies with SOCKS5 "no authentication required" (method 0x00).
mock_socks5_open() {
  local port="$1"
  local tmp
  tmp="$(mktemp)"
  printf '\x05\x00' >"$tmp"
  mock_raw_reply "$port" "$tmp"
}

# mock_socks5_denied <port>
# Replies with SOCKS5 "no acceptable methods" (0xFF).
mock_socks5_denied() {
  local port="$1"
  local tmp
  tmp="$(mktemp)"
  printf '\x05\xff' >"$tmp"
  mock_raw_reply "$port" "$tmp"
}

# no listener at all on <port> -> servauth should see connection refused
# and classify the check as "closed". Nothing to start here, just document
# that the caller should pick a port with nothing listening on it.
mock_closed() { :; }

cleanup_mocks() {
  local pid
  for pid in $MOCK_PIDS; do
    [ -n "$pid" ] && kill "$pid" 2>/dev/null
  done
  MOCK_PIDS=""
}

# ---------------------------------------------------------------------------
# tiny test-reporting helpers
# ---------------------------------------------------------------------------
TESTS_RUN=0
TESTS_FAILED=0

pass() {
  TESTS_RUN=$((TESTS_RUN + 1))
  echo "ok - $*"
}

fail() {
  TESTS_RUN=$((TESTS_RUN + 1))
  TESTS_FAILED=$((TESTS_FAILED + 1))
  echo "not ok - $*"
}

report_and_exit() {
  echo "# $TESTS_RUN run, $((TESTS_RUN - TESTS_FAILED)) passed, $TESTS_FAILED failed"
  [ "$TESTS_FAILED" -eq 0 ]
}
