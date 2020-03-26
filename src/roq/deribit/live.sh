#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

NAME="deribit"

CONFIG_FILE="$CWD/config/$NAME.toml"

URI="deribit.com"

WS_URI="wss://$URI/ws/api/v2"
FIX_URI="tcp://www.$URI:9880"

$PREFIX ./roq-deribit \
	--name "deribit" \
	--config-file "$CONFIG_FILE" \
	--ws-uri "$WS_URI" \
	--fix-uri "$FIX_URI" \
	--listen "$CWD/$NAME.sock" \
	--metrics "$CWD/${NAME}_metrics.sock" \
	$@
