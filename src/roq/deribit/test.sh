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
# WS_URI="wss://$URI/ws/api/v2"

$PREFIX ./roq-deribit \
	--name "$NAME" \
	--config-file "$CONFIG_FILE" \
	--fix-uri "$FIX_URI" \
	--listen "$CWD/$NAME.sock" \
	--metrics "$CWD/$NAME_metrics.sock" \
	--inter-thread-queue-size 65536 \
	--inter-process-queue-size 262144 \
	--broadcast-queue-size 262144 \
	$@
