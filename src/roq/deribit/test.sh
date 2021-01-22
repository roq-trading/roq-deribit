#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

NAME="deribit-test"

CONFIG_FILE="$CWD/config/$NAME.toml"

URI="test.deribit.com"

FIX_URI="tcp://$URI:9881"
WS_URI="wss://$URI/ws/api/v2"

$PREFIX ./roq-deribit \
	--name "deribit" \
	--config_file "$CONFIG_FILE" \
	--client_listen_address "$CWD/$NAME.sock" \
	--metrics_listen_address "$CWD/${NAME}_metrics.sock" \
	--fix_uri "$FIX_URI" \
	--ws_uri "$WS_URI" \
	$@
