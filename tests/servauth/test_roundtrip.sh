#!/bin/sh
#
# test_roundtrip.sh - exercises the three core servauth checks end to end:
#
#   1. dns forward  - hostname -> address, via the system resolver
#                      (servauth reads nameservers from /etc/resolv.conf)
#   2. dns reverse   - address -> hostname, same resolver
#   3. proxy scan    - connects out to a (netcat-mocked) proxy and checks
#                      whether it works as an open relay
#
# Run directly: ./test_roundtrip.sh
# Or via run_tests.sh for the whole suite.
#
# Plain POSIX sh - tested under dash and shish/ash, not just bash.

set -u

cd "$(dirname "$0")"
. ./common.sh

# Hostname/address used for the DNS round trip. Override with:
#   TEST_FORWARD_HOST=example.com TEST_REVERSE_ADDR=93.184.216.34 ./test_roundtrip.sh
TEST_FORWARD_HOST=${TEST_FORWARD_HOST:-localhost}
TEST_REVERSE_ADDR=${TEST_REVERSE_ADDR:-127.0.0.1}

PROXY_PORT=${PROXY_PORT:-18080}

CLEANUP_DONE=0
cleanup() {
  [ "$CLEANUP_DONE" = 1 ] && return 0
  CLEANUP_DONE=1
  cleanup_mocks
  sv_stop
}
# INT/TERM only, not EXIT: some ash-family shells (shish included) run the
# EXIT trap for every subshell forked to run a pipeline (e.g. `foo | bar`),
# not just once when the top-level shell actually exits - which would kill
# servauth mid-test the first time this script pipes anything through
# awk/sed. Call cleanup explicitly at each real exit point instead.
trap cleanup INT TERM

sv_start || { echo "could not start servauth" >&2; cleanup; exit 1; }

# --- 1. dns forward --------------------------------------------------------
sv_send "dns forward 1 $TEST_FORWARD_HOST"

reply=$(sv_expect "dns forward 1")
if [ -n "$reply" ]; then
  addr=$(printf '%s' "$reply" | awk '{print $NF}')
  # loose dotted-decimal check: only digits/dots, at least 3 dots, no
  # leading/trailing/doubled dot - good enough to tell "an address came
  # back" from "no address came back" for this smoke test.
  case "$addr" in
    ''|*[!0-9.]*|.*|*.|*..*) ip_like=0 ;;
    *.*.*.*) ip_like=1 ;;
    *) ip_like=0 ;;
  esac
  if [ "$addr" != "1" ] && [ "$ip_like" = "1" ]; then
    pass "dns forward: $TEST_FORWARD_HOST -> $addr"
  else
    fail "dns forward: got reply with no address ('$reply') - resolver may be unreachable"
  fi
else
  fail "dns forward: no reply from servauth within ${SV_TIMEOUT}s"
fi

# --- 2. dns reverse ---------------------------------------------------------
sv_send "dns reverse 2 $TEST_REVERSE_ADDR"

reply=$(sv_expect "dns reverse 2")
if [ -n "$reply" ]; then
  name=$(printf '%s' "$reply" | awk '{print $NF}')
  if [ "$name" != "2" ] && [ -n "$name" ]; then
    pass "dns reverse: $TEST_REVERSE_ADDR -> $name"
  else
    fail "dns reverse: got reply with no hostname ('$reply')"
  fi
else
  fail "dns reverse: no reply from servauth within ${SV_TIMEOUT}s"
fi

# --- 3. proxy scan -----------------------------------------------------------
mock_http_open "$PROXY_PORT"

sv_send "proxy 3 127.0.0.1:$PROXY_PORT 127.0.0.1:6667 http"

reply=$(sv_expect "proxy 3")
if [ "$reply" = "proxy 3 open" ]; then
  pass "proxy scan: mocked open HTTP proxy on :$PROXY_PORT correctly detected as open"
else
  fail "proxy scan: expected 'proxy 3 open', got '${reply:-<no reply>}'"
fi

cleanup
report_and_exit
