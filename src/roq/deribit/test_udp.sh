#!/usr/bin/env bash

NAME="deribit"

CONFIG="${CONFIG:-$NAME-test}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG.toml"

SECRETS_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG-secrets.toml"

URI="test.deribit.com"

FIX_URI="tcp://$URI:9881"
WS_URI="wss://$URI/ws/api/v2"

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
  --secrets_file "$SECRETS_FILE" \
  --cache_dir "$HOME/var/lib/roq/cache" \
  --event_log_dir "$HOME/var/lib/roq/data" \
  --event_log_symlink true \
  --client_listen_address "$HOME/run/$NAME.sock" \
  --service_listen_address "$HOME/run/metrics/${NAME}.sock" \
  --fix_uri "$FIX_URI" \
  --ws_uri "$WS_URI" \
  --udp_encoding "flatbuffers" \
  --udp_snapshot_port 5678 \
  --udp_incremental_port 6789 \
  $@
