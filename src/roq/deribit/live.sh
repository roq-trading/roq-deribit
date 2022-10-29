#!/usr/bin/env bash

NAME="deribit"

CONFIG_FILE="config/$NAME.toml"

URI="deribit.com"

FIX_URI="tcp://www.$URI:9881"
WS_URI="wss://www.$URI/ws/api/v2"

# debug?

if [ "$1" == "debug" ]; then
  KERNEL="$(uname -a)"
  case "$KERNEL" in
    Linux*)
      PREFIX="gdb --args"
      ;;
    Darwin*)
      PREFIX="lldb --"
      ;;
  esac
  shift 1
else
  PREFIX=
fi

# launch

$PREFIX "./roq-deribit" \
  --name "$NAME" \
  --config_file "$CONFIG_FILE" \
  --cache_dir "$HOME/var/lib/roq/cache" \
  --event_log_dir "$HOME/var/lib/roq/data" \
  --event_log_symlink true \
  --client_listen_address "$HOME/run/$NAME.sock" \
  --service_listen_address "$HOME/run/metrics/${NAME}.sock" \
  --fix_uri "$FIX_URI" \
  --ws_uri "$WS_URI" \
  $@
