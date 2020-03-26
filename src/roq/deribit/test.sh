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

WS_URI="wss://$URI/ws/api/v2"
FIX_URI="tcp://$URI:9881"

$PREFIX ./roq-deribit \
	--name "deribit" \
	--config-file "$CONFIG_FILE" \
	--ws-uri "$WS_URI" \
	--fix-uri "$FIX_URI" \
	--listen "$CWD/$NAME.sock" \
	$@
