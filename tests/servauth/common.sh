#!/usr/bin/env bash
#
# common.sh - shared helpers for servauth unit tests
#
# Provides:
#   - sv_start / sv_stop        : launch/kill servauth as a coprocess and
#                                  talk to it over its stdin/stdout control
#                                  protocol (the same protocol ircd uses
#                                  over the socketpair)
#   - sv_send / sv_expect       : send a command line, wait for a reply
#   - mock_proxy_*              : spawn netcat listeners that speak just
#                                  enough of each proxy protocol to make
#                                  servauth classify them as open/denied/etc
#   - cleanup_mocks             : kill any netcat listeners started by a test
#
# Every test script should `source` this file, call sv_start once, run its
# checks, then call sv_stop (trap on EXIT is recommended, see test_*.sh).

set -u

SV_TIMEOUT=${SV_TIMEOUT:-5}

# ---------------------------------------------------------------------------
# locate the servauth binary
# ---------------------------------------------------------------------------
sv_find_binary() {
  if [ -n "${SERVAUTH_BIN:-}" ] && [ -x "$SERVAUTH_BIN" ]; then
    echo "$SERVAUTH_BIN"
    return 0
  fi

  local here
  here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

  local candidates=(
    "$here/../../build/x86_64-linux-debug/lib/servauth/servauth"
    "$here/../../build/x86_64-linux-gnu/lib/servauth/servauth"
    "$here/../../build/x86_64-linux-clang/lib/servauth/servauth"
    "/usr/local/libexec/servauth"
  )

  local c
  for c in "${candidates[@]}"; do
    if [ -x "$c" ]; then
      echo "$c"
      return 0
    fi
  done

  return 1
}

# ---------------------------------------------------------------------------
# start/stop servauth as a coprocess.
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

  # `exec` inside the coproc body so bash's tracked PID is servauth's own
  # pid (not a wrapper shell's) - otherwise `kill "$SV_PID"` in sv_stop
  # misses the real process and leaks a busy-looping orphan.
  coproc SV_PROC { exec "$bin"; }

  SV_IN=${SV_PROC[1]}
  SV_OUT=${SV_PROC[0]}
  SV_PID=$SV_PROC_PID

  # give it a moment to finish servauth_init()
  sleep 0.2

  if ! kill -0 "$SV_PID" 2>/dev/null; then
    echo "sv_start: servauth ($bin) exited immediately" >&2
    return 1
  fi

  return 0
}

sv_stop() {
  [ -n "${SV_PID:-}" ] || return 0

  # belt-and-suspenders: also reap any direct child (in case some bash
  # build didn't exec(3) in place for the coproc as expected)
  local child
  child=$(pgrep -P "$SV_PID" 2>/dev/null)

  kill "$SV_PID" 2>/dev/null
  [ -n "$child" ] && kill "$child" 2>/dev/null

  wait "$SV_PID" 2>/dev/null

  unset SV_PID SV_IN SV_OUT
}

# sv_send <command line...>
#
# One line == one command, exactly as sent by ircd's sauth_* functions.
# NOTE: servauth treats an unrecognised or malformed command as fatal and
# shuts itself down - keep commands well-formed (see servauth's
# lib/servauth/commands.c for exact argument counts).
sv_send() {
  echo "$*" >&"$SV_IN"
}

# sv_expect <prefix> [timeout]
#
# Reads lines from servauth until one starts with <prefix> (space-anchored,
# so "dns forward 1" won't match "dns forward 12"), or the timeout elapses.
# Echoes the matching line on success, returns 1 on timeout/EOF.
sv_expect() {
  local prefix="$1"
  local timeout="${2:-$SV_TIMEOUT}"
  local line
  local deadline=$(( $(date +%s) + timeout ))

  while [ "$(date +%s)" -le "$deadline" ]; do
    if IFS= read -r -t "$timeout" -u "$SV_OUT" line; then
      case "$line" in
        "$prefix "*|"$prefix")
          echo "$line"
          return 0
          ;;
        *)
          # not the reply we're waiting for (could be an unrelated/late
          # reply from a previous query) - keep waiting
          ;;
      esac
    else
      return 1
    fi
  done

  return 1
}

# ---------------------------------------------------------------------------
# proxy mocks
#
# servauth connects out to the address given in the "proxy" command, writes
# a protocol-specific probe, then classifies the reply. These helpers start
# a one-shot netcat listener that plays the server side of that protocol.
# ---------------------------------------------------------------------------
declare -a MOCK_PIDS=()

# mock_raw_reply <port> <bytes-file>
#
# Generic helper: listen once on <port>, send the contents of <bytes-file>
# to whoever connects, then exit. Does not attempt to read/validate the
# client's request - servauth only cares about what comes back.
mock_raw_reply() {
  local port="$1"
  local file="$2"

  nc -l -p "$port" -q1 <"$file" >/dev/null 2>&1 &
  MOCK_PIDS+=("$!")

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
  for pid in "${MOCK_PIDS[@]:-}"; do
    [ -n "$pid" ] && kill "$pid" 2>/dev/null
  done
  MOCK_PIDS=()
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
