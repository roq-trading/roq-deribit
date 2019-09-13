#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

CONFIG_FILE="$CWD/config/live.toml"

WS_URI="wss://deribit.com/ws/api/v2"
FIX_URI="tcp://www.deribit.com:9880"

$PREFIX ./roq-deribit \
	--name "deribit" \
	--metrics $CWD/metrics.sock \
	--config-file "$CONFIG_FILE" \
	--ws-uri "$WS_URI" \
	--fix-uri "$FIX_URI" \
	$@
