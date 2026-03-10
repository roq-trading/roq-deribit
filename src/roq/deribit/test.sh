#!/usr/bin/env bash

NAME="deribit"

CONFIG="${CONFIG:-$NAME-test}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG.toml"

SECRETS_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG-secrets.toml"

FLAGFILE="../../../share/flags/test/flags.cfg"

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

DATABASE_URI="http://192.168.188.70:8123"
#DATABASE_URI="http://localhost:8123"

# launch

$PREFIX "./roq-deribit" \
  --name "$NAME" \
  --config_file "$CONFIG_FILE" \
  --secrets_file "$SECRETS_FILE" \
  --flagfile "$FLAGFILE" \
  --cache_dir "$HOME/var/lib/roq/cache" \
  --event_log_dir "$HOME/var/lib/roq/data" \
  --client_listen_address "unix://$HOME/run/$NAME.sock" \
  --service_listen_address "unix://$HOME/run/metrics/$NAME.sock" \
  --download_trades_lookback=5m \
  --download_trades_lookback_on_restart=24h \
  $@
#  --oms_cache=true \
#  --oms_multicast_port 1234 \
#  --oms_multicast_address=224.1.1.1 \
#  --oms_local_interface="$LOCAL_INTERFACE" \
#  --oms_multicast_ttl 4 \
#  --oms_multicast_loop=true \
#  --oms_listen_port 9876 \
#  --cache_database_uri "$DATABASE_URI" \
#  --cache_database_name "roq" \
#  --fix_cancel_on_disconnect=false \
#  --enable_portfolio=true \
#  --time_series_interval "60s" \
#  --time_series_lookback "0s" \
#  --time_series_realtime=true \
#  $@
