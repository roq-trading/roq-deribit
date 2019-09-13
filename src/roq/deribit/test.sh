#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

CONFIG_FILE="$CWD/config/test.toml"

WS_URI="wss://test.deribit.com/ws/api/v2"
FIX_URI="tcp://test.deribit.com:9881"

$PREFIX ./roq-deribit \
	--name "deribit" \
	--metrics $CWD/metrics.sock \
	--config-file "$CONFIG_FILE" \
	--ws-uri "$WS_URI" \
	--fix-uri "$FIX_URI" \
	$@
