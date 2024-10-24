#!/usr/bin/env bash

NAME="deribit"

CONFIG="${CONFIG:-$NAME-test}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG.toml"

SECRETS_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG-secrets.toml"

URI="test.deribit.com"

FIX_URI="tcp://$URI:9881"
WS_URI="wss://$URI/ws/api/v2"

KERNEL="$(uname -a)"

case "$KERNEL" in
  Linux*)
    LOCAL_INTERFACE=$(ip route get 8.8.8.8 | sed -n 's/.*src \([^\ ]*\).*/\1/p')
    ;;
  Darwin*)
    LOCAL_INTERFACE=$(osascript -e "IPv4 address of (system info)")
    ;;
  *)
    (>&2 echo -e "\033[1;31mERROR: Unknown architecture.\033[0m") && exit 1
esac

# debug?

if [ "$1" == "debug" ]; then
  case "$KERNEL" in
    Linux*)
      PREFIX="gdb --command=gdb_commands --args"
      ;;
    Darwin*)
      PREFIX="lldb --"
      ;;
  esac
  shift 1
else
  PREFIX=
fi

echo "LOCAL_INTERFACE=$LOCAL_INTERFACE"

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
  --download_trades_lookback=5m \
  --download_trades_lookback_on_restart=24h \
  --multicast_config_file "$HOME/dev/roq-dev/roq-deribit/share/prod/channels.json" \
  --multicast_channel_ids 1,3,4,6,21 \
  --multicast_local_interface "$LOCAL_INTERFACE" \
  $@
