#!/bin/sh
#
# test_ssl_listener.sh - verifies the chaosircd SSL/TLS listener (default
# port 6697) actually completes a TLS handshake and lets a client register.
#
# Drives the connection with `openssl s_client`, talking to it over a pair
# of named pipes (POSIX sh has no coprocess support), sends NICK/USER, and
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
#
# Runs under any POSIX shell (tested with dash and shish/ash), not just bash.

set -u

HOST=${1:-${IRCD_HOST:-127.0.0.1}}
PORT=${2:-${IRCD_PORT:-6697}}
NICK=${IRCD_NICK:-ssltest$$}
TIMEOUT=${IRCD_TIMEOUT:-15}

TMPDIR=$(mktemp -d)
IN_FIFO="$TMPDIR/in"
OUT_FIFO="$TMPDIR/out"
HANDSHAKE_LOG="$TMPDIR/handshake.log"
READER="$TMPDIR/reader.sh"
mkfifo "$IN_FIFO" "$OUT_FIFO"

log_in()  { echo "<< $*"; }
log_out() { echo ">> $*"; }

# The read loop needs a per-run timeout, but POSIX `read` has no -t option
# (dash/ash don't support it), so the loop is farmed out to a small helper
# script and bounded with the external `timeout` command instead. Built
# with `printf`, not a heredoc, and written here - before the fifos below
# are opened and the connection backgrounded: some ash-family shells
# (shish included) mis-parse a heredoc that has much script content after
# it, and separately corrupt a later `>` file redirection (silently
# sending its output to stdout instead) once those fifo fds are live.
printf '%s\n' \
  '#!/bin/sh' \
  'while IFS= read -r line; do' \
  '  line=$(printf "%s" "$line" | tr -d "\r")' \
  '  printf "IN:%s\n" "$line"' \
  '  case "$line" in' \
  '    PING*)' \
  '      echo "RESULT:ping cookie challenge received (lc_cookie active)"' \
  '      exit 0' \
  '      ;;' \
  '  esac' \
  '  code=$(printf "%s" "$line" | awk "{print \$2}")' \
  '  if [ "$code" = "376" ] || [ "$code" = "422" ]; then' \
  '    echo "RESULT:end of MOTD (numeric $code)"' \
  '    exit 0' \
  '  fi' \
  'done' \
  'exit 1' \
  >"$READER"
chmod +x "$READER"

CLEANUP_DONE=0
cleanup() {
  [ "$CLEANUP_DONE" = 1 ] && return 0
  CLEANUP_DONE=1
  [ -n "${CONN_PID:-}" ] && kill "$CONN_PID" 2>/dev/null
  exec 8>&- 2>/dev/null
  exec 9<&- 2>/dev/null
  rm -rf "$TMPDIR"
}
# INT/TERM only, not EXIT: some ash-family shells (shish included) run the
# EXIT trap for every subshell forked to run a pipeline (e.g. `foo | bar`),
# not just once when the top-level shell actually exits - which would kill
# the openssl connection mid-script the first time this script pipes
# anything through awk/sed. Call cleanup explicitly at each real exit point
# instead.
trap cleanup INT TERM

echo "# connecting to $HOST:$PORT with TLS..."

# openssl opens its own fd for each fifo path (via the < and >
# redirections below) rather than inheriting a dup of a fd we already have
# open - openssl does its own fcntl(F_SETFL) for non-blocking I/O, and a
# dup would share that file *description* with our fds, silently making
# our reads/writes non-blocking too. Separate open() calls on the same
# fifo path get independent descriptions, so openssl's flag changes stay
# local to its own end.
#
# Opening a fifo for one direction blocks until a peer opens the other
# end, so the two sides must be opened in matching order: openssl's
# stdin/stdout opens happen as part of backgrounding it below, and the
# `exec` lines right after unblock each of those opens in turn.
#
# fd 8/9, not 3/4: at least one ash-family shell (shish) uses a low fd
# internally to keep reading the running script off disk, and reassigning
# it out from under the interpreter via `exec 3>...` corrupts its parsing
# of everything after that point in maddeningly inconsistent ways. Staying
# well above the fds a shell would plausibly want for its own bookkeeping
# avoids the whole class of bug.
#
# -quiet keeps the TLS handshake chatter off stdout so we can read clean
# IRC protocol lines; -crlf sends our writes as CRLF, as IRC requires.
# Handshake diagnostics (cipher, cert subject, verify result) go to a
# separate log we print afterwards instead of interleaving with traffic.
openssl s_client -connect "$HOST:$PORT" -quiet -crlf <"$IN_FIFO" >"$OUT_FIFO" 2>"$HANDSHAKE_LOG" &
CONN_PID=$!

exec 8>"$IN_FIFO"
exec 9<"$OUT_FIFO"

sleep 0.5

if ! kill -0 "$CONN_PID" 2>/dev/null; then
  echo "not ok - could not establish a TLS connection to $HOST:$PORT"
  sed 's/^/# /' "$HANDSHAKE_LOG"
  cleanup
  exit 1
fi

echo "# TLS handshake:"
sed 's/^/# /' "$HANDSHAKE_LOG"

send() {
  log_out "$*"
  printf '%s\n' "$*" >&8
}

send "NICK $NICK"
send "USER $NICK 0 * :SSL listener test"

success=0
reason=""

OUTFILE="$TMPDIR/reader_output"
timeout "$TIMEOUT" "$READER" <&9 >"$OUTFILE"
status=$?

# Read via redirection, not a pipe: some ash-family shells (shish
# included) run trap handlers early when a piped subshell exits, which
# would tear down the still-needed connection mid-script.
while IFS= read -r outline; do
  case "$outline" in
    IN:*) log_in "${outline#IN:}" ;;
  esac
done <"$OUTFILE"

if [ "$status" -eq 0 ]; then
  success=1
  reason=$(sed -n 's/^RESULT://p' "$OUTFILE" | tail -n1)
fi

send "QUIT :test complete"
sleep 0.2

if [ "$success" -eq 1 ]; then
  echo "ok - TLS listener on $HOST:$PORT: $reason"
  cleanup
  exit 0
else
  echo "not ok - TLS listener on $HOST:$PORT: no ping cookie or end-of-MOTD within ${TIMEOUT}s"
  cleanup
  exit 1
fi
