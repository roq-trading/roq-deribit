#!/usr/bin/env bash

NAME="deribit-test"

CONFIG_FILE="config/$NAME.toml"

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
	--name "deribit" \
	--config_file "$CONFIG_FILE" \
	--client_listen_address "$NAME.sock" \
	--metrics_listen_address "${NAME}_metrics.sock" \
	--fix_uri "$FIX_URI" \
	--ws_uri "$WS_URI" \
	$@
