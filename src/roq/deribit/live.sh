#!/usr/bin/env bash

NAME="deribit"

CONFIG="${CONFIG:-$NAME}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG.toml"

URI="deribit.com"

FIX_URI="tcp://fix.$URI:9881"
WS_URI="wss://www.$URI/ws/api/v2"
REST_URI="https://www.$URI"

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
  --rest_uri "$REST_URI" \
  --cache_all_reference_data true \
  --time_series_interval "60s" \
  --time_series_lookback "2h" \
  --time_series_realtime true \
  --download_time_series true \
  $@
