#!/usr/bin/env bash

CONFIG_FILE="$HOME/dev/roq-dev/roq-deribit/share/prod/channels.json" 

./roq-deribit-filter \
  --type tcpdump \
  --multicast_config_file $CONFIG_FILE \
  --multicast_channel_ids 1,2,3,4,7,10,13,21
