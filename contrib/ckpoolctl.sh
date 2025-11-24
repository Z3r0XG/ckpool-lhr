#!/usr/bin/env bash
# Simple start/stop wrapper for running ckpool from repository for development.
# Usage: ./contrib/ckpoolctl.sh start|stop|status|fg

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CKPOOL_BIN="${ROOT_DIR}/src/ckpool"
# Allow overriding the config file via environment (for local testing)
CKPOOL_CONF="${CKPOOL_CONF:-${ROOT_DIR}/ckpool.conf}"
PIDFILE="${ROOT_DIR}/logs/ckpool.pid"
OUTFILE="${ROOT_DIR}/logs/ckpool.out"

mkdir -p "${ROOT_DIR}/logs"

start() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat $PIDFILE)" 2>/dev/null; then
    echo "ckpool already running (pid $(cat $PIDFILE))"
    exit 0
  fi
  echo "Starting ckpool..."
  nohup "$CKPOOL_BIN" -c "$CKPOOL_CONF" > "$OUTFILE" 2>&1 &
  echo $! > "$PIDFILE"
  echo "Started pid $(cat $PIDFILE)"
}

stop() {
  if [ ! -f "$PIDFILE" ]; then
    echo "No pidfile found"
    exit 0
  fi
  pid=$(cat "$PIDFILE")
  echo "Stopping ckpool pid $pid"
  kill "$pid" || true
  sleep 1
  if kill -0 "$pid" 2>/dev/null; then
    echo "Forcing kill"
    kill -9 "$pid" || true
  fi
  rm -f "$PIDFILE"
}

status() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat $PIDFILE)" 2>/dev/null; then
    echo "ckpool running pid $(cat $PIDFILE)"
  else
    echo "ckpool not running"
  fi
}

fg() {
  exec "$CKPOOL_BIN" -c "$CKPOOL_CONF"
}

case "$1" in
  start) start ;; 
  stop) stop ;; 
  status) status ;; 
  fg) fg ;; 
  *) echo "Usage: $0 {start|stop|status|fg}"; exit 2 ;;
esac
