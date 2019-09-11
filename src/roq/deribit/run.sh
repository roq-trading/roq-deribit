#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

CONFIG_DIR="$CWD/../../../share/"

$PREFIX ./roq-deribit \
	--name "deribit" \
	--config-directory "$CONFIG_DIR" \
	--config-file master.conf \
	--config-variables variables.conf \
	--metrics $CWD/metrics.sock \
	$@
