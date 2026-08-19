#!/usr/bin/env bash

NAME="deribit"

CONFIG="${CONFIG:-$NAME-test}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG.toml"

SECRETS_FILE="$ROQ_CONFIG_PATH/roq-deribit/$CONFIG-secrets.toml"

FLAGFILE="../../../../share/flags/test/flags.cfg"

KERNEL="$(uname -a)"

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


$PREFIX "./roq-deribit-fix-bridge" \
  --name "$NAME" \
  --config_file "$CONFIG_FILE" \
  --secrets_file "$SECRETS_FILE" \
  --flagfile "$FLAGFILE" \
  --cache_dir "$HOME/var/lib/roq/cache" \
  --event_log_dir "$HOME/var/lib/roq/data" \
  --client_listen_address "tcp://localhost:1234" \
  $@
