#!/usr/bin/env bash
#
# test_ssl_listener.sh - verifies the chaosircd SSL/TLS listener (default
# port 6697) actually completes a TLS handshake and lets a client register.
#
# Drives the connection with `openssl s_client` (as a bash coprocess) so we
# get a real TLS session without any extra tooling, sends NICK/USER, and
# dumps every line sent/received to the console (">> " = sent, "<< " =
# received). The test is considered passed as soon as it sees either:
#
#   - a PING challenge (the [lc_cookie] module's ping-cookie check, if
#     that module is loaded), or
#   - numeric 376 (End of MOTD) / 422 (MOTD File is missing), if it's not
#
# whichever the server actually does first - so the same script works
# whether or not lc_cookie is enabled in modules.conf.
#
# Usage:
#   ./test_ssl_listener.sh [host] [port]
#   IRCD_HOST=irc.example.com IRCD_PORT=6697 ./test_ssl_listener.sh

set -u

HOST=${1:-${IRCD_HOST:-127.0.0.1}}
PORT=${2:-${IRCD_PORT:-6697}}
NICK=${IRCD_NICK:-ssltest$$}
TIMEOUT=${IRCD_TIMEOUT:-15}

HANDSHAKE_LOG=$(mktemp)

log_in()  { echo "<< $*"; }
log_out() { echo ">> $*"; }

cleanup() {
  [ -n "${CONN_PID:-}" ] && kill "$CONN_PID" 2>/dev/null
  rm -f "$HANDSHAKE_LOG"
}
trap cleanup EXIT

echo "# connecting to $HOST:$PORT with TLS..."

# -quiet keeps the TLS handshake chatter off stdout so we can read clean
# IRC protocol lines; -crlf sends our writes as CRLF, as IRC requires.
# Handshake diagnostics (cipher, cert subject, verify result) go to a
# separate log we print afterwards instead of interleaving with traffic.
coproc TLS { openssl s_client -connect "$HOST:$PORT" -quiet -crlf 2>"$HANDSHAKE_LOG"; }

TLS_IN=${TLS[1]}
TLS_OUT=${TLS[0]}
# openssl may exit almost immediately on a failed connect, in which case
# bash can reap the coproc job and unset its auto ${TLS_PID} variable at
# any point afterwards - copy it into a differently-named variable right
# away (assigning back to TLS_PID itself would be a no-op: it's the same
# variable bash keeps clearing).
CONN_PID=${TLS_PID:-}

sleep 0.5

if [ -z "$CONN_PID" ] || ! kill -0 "$CONN_PID" 2>/dev/null; then
  echo "not ok - could not establish a TLS connection to $HOST:$PORT"
  sed 's/^/# /' "$HANDSHAKE_LOG"
  exit 1
fi

echo "# TLS handshake:"
sed 's/^/# /' "$HANDSHAKE_LOG"

send() {
  log_out "$*"
  printf '%s\n' "$*" >&"$TLS_IN"
}

send "NICK $NICK"
send "USER $NICK 0 * :SSL listener test"

deadline=$(( $(date +%s) + TIMEOUT ))
success=0
reason=""

while [ "$(date +%s)" -le "$deadline" ]; do
  if ! IFS= read -r -t "$TIMEOUT" -u "$TLS_OUT" line; then
    break
  fi

  line=${line%$'\r'}
  log_in "$line"

  case "$line" in
    PING*)
      success=1
      reason="ping cookie challenge received (lc_cookie active)"
      break
      ;;
  esac

  # numeric replies: ":server <code> <target> ..."
  code=$(awk '{print $2}' <<<"$line")
  if [ "$code" = "376" ] || [ "$code" = "422" ]; then
    success=1
    reason="end of MOTD (numeric $code)"
    break
  fi
done

send "QUIT :test complete"
sleep 0.2

if [ "$success" -eq 1 ]; then
  echo "ok - TLS listener on $HOST:$PORT: $reason"
  exit 0
else
  echo "not ok - TLS listener on $HOST:$PORT: no ping cookie or end-of-MOTD within ${TIMEOUT}s"
  exit 1
fi
