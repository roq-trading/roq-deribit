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

FIX_URI="tcp://www.$URI:9880"
# WS_URI="wss://$URI/ws/api/v2"

$PREFIX ./roq-deribit \
	--name "$NAME" \
	--config-file "$CONFIG_FILE" \
	--fix-uri "$FIX_URI" \
	--listen "$CWD/$NAME.sock" \
	--metrics "$CWD/$NAME_metrics.sock" \
	$@
