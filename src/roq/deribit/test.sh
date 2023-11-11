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
  --event_log_symlink=true \
  --client_listen_address "$HOME/run/$NAME.sock" \
  --service_listen_address "$HOME/run/metrics/$NAME.sock" \
  --fix_uri "$FIX_URI" \
  --ws_uri "$WS_URI" \
  --cache_all_reference_data=true \
  --download_trades_lookback=24h \
  --oms_cache=true \
  --oms_multicast_port 1234 \
  --oms_multicast_address=224.1.1.1 \
  --oms_local_interface 192.168.188.64 \
  --oms_multicast_ttl 4 \
  --oms_multicast_loop=true \
  --oms_listen_port 9876 \
  --cache_database_uri "http://192.168.188.70:8123" \
  --cache_database_name "roq_cache" \
  --fix_cancel_on_disconnect=false \
  $@
