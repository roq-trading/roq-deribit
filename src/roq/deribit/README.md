# ReferenceData

## SPOT

### FIX

* `base_currency` <= `currency` (BTC)
* `quote_currency` <= `price_quote_currency` (USDT)
* `settlement_currency` <= `settl_currency` (??? USDT ???)

```
{
  symbol="BTC_USDT",
  security_desc="Crypto Spot",
  security_type="FXSPOT",
  put_or_call=<UNDEFINED>,
  strike_price=nan,
  strike_currency="",
  currency="BTC",
  instrument_price_precision=4,
  min_price_increment=1,
  underlying_symbol="",
  issue_date=1702374650000ms,
  maturity_date=32503680000000ms,
  maturity_time="08:00:00+00:00",
  min_trade_vol=1,
  settl_type=REGULAR,
  settl_currency="USDT",
  comm_currency="BTC_USDT",
  contract_multiplier=0.00010000000000000003,
  security_status="",
  price_quote_currency="USDT"
}
```

### REST

```
{
  "tick_size_steps": [],
  "quote_currency": "USDT",
  "min_trade_amount": 0.0001,
  "counter_currency": "USDT",
  "block_trade_min_trade_amount": 10,
  "block_trade_commission": 0,
  "instrument_id": 282196,
  "base_currency": "BTC",
  "block_trade_tick_size": 0.0001,
  "tick_size": 1,
  "contract_size": 0.0001,
  "is_active": true,
  "expiration_timestamp": 32503708800000,
  "instrument_type": "linear",
  "taker_commission": 0,
  "maker_commission": 0,
  "instrument_name": "BTC_USDT",
  "creation_timestamp": 1702374650000,
  "kind": "spot",
  "rfq": false,
  "price_index": "btc_usdt"
}
```

## LINEAR

### FIX

* `base_currency` <= ??? (BTC)
* `quote_currency` <= `currency` (USDC)
* `settlement_currency` = `settl_currency` (USDC)

```
{
  symbol="BTC_USDC-PERPETUAL",
  security_desc="future",
  security_type="FUT",
  put_or_call=<UNDEFINED>,
  strike_price=nan,
  strike_currency="",
  currency="USDC",
  instrument_price_precision=4,
  min_price_increment=1,
  underlying_symbol="",
  issue_date=1646824342000ms,
  maturity_date=32503680000000ms,
  maturity_time="08:00:00+00:00",
  min_trade_vol=1,
  settl_type=REGULAR,
  settl_currency="USDC",
  comm_currency="USDC",
  contract_multiplier=0.0010000000000000002,
  security_status="",
  price_quote_currency=""
}
```

### REST

```
{
  "tick_size_steps": [],
  "quote_currency": "USDC",
  "min_trade_amount": 0.001,
  "counter_currency": "USDC",
  "settlement_currency": "USDC",
  "block_trade_min_trade_amount": 200000,
  "block_trade_commission": 0.0001,
  "max_liquidation_commission": 0.0075,
  "max_leverage": 50,
  "future_type": "linear",
  "settlement_period": "perpetual",
  "instrument_id": 211704,
  "base_currency": "BTC",
  "block_trade_tick_size": 1,
  "tick_size": 1,
  "contract_size": 0.001,
  "is_active": true,
  "expiration_timestamp": 32503708800000,
  "instrument_type": "linear",
  "taker_commission": 0.0005,
  "maker_commission": 0,
  "instrument_name": "BTC_USDC-PERPETUAL",
  "creation_timestamp": 1646824342000,
  "kind": "future",
  "rfq": false,
  "price_index": "btc_usdc"
}
```

## INVERSE

### FIX

* `base_currency` <= ??? (BTC)
* `quote_currency` <= `currency` (USD)
* `settlement_currency` = ??? (USD)

```
{
  symbol="BTC-PERPETUAL",
  security_desc="future",
  security_type="FUT",
  put_or_call=<UNDEFINED>,
  strike_price=nan,
  strike_currency="",
  currency="USD",
  instrument_price_precision=2,
  min_price_increment=0.5,
  underlying_symbol="",
  issue_date=1534242287000ms,
  maturity_date=32503680000000ms,
  maturity_time="08:00:00+00:00",
  min_trade_vol=1,
  settl_type=REGULAR,
  settl_currency="USD",
  comm_currency="BTC",
  contract_multiplier=10,
  security_status="",
  price_quote_currency=""
}
```

### REST

```
{
  "tick_size_steps": [],
  "quote_currency": "USD",
  "min_trade_amount": 10,
  "counter_currency": "USD",
  "settlement_currency": "BTC",
  "block_trade_min_trade_amount": 200000,
  "block_trade_commission": 0.00025,
  "max_liquidation_commission": 0.0075,
  "max_leverage": 50,
  "future_type": "reversed",
  "settlement_period": "perpetual",
  "instrument_id": 210838,
  "base_currency": "BTC",
  "block_trade_tick_size": 0.01,
  "tick_size": 0.5,
  "contract_size": 10,
  "is_active": true,
  "expiration_timestamp": 32503708800000,
  "instrument_type": "reversed",
  "taker_commission": 0.0005,
  "maker_commission": 0,
  "instrument_name": "BTC-PERPETUAL",
  "creation_timestamp": 1534242287000,
  "kind": "future",
  "rfq": false,
  "price_index": "btc_usd"
}
```

## OPTION

### FIX

```
{
  symbol="BTC-27DEC24-62000-P",
  security_desc="option",
  security_type="OPT",
  put_or_call=PUT,
  strike_price=62000,
  strike_currency="USD",
  currency="BTC",
  instrument_price_precision=4,
  min_price_increment=0.00010000000000000003,
  underlying_symbol="BTC-27DEC24",
  issue_date=1704753480000ms,
  maturity_date=1735257600000ms,
  maturity_time="08:00:00+00:00",
  min_trade_vol=0.1,
  settl_type=<UNKNOWN>,
  settl_currency="USD",
  comm_currency="BTC",
  contract_multiplier=1,
  security_status="",
  price_quote_currency=""
}
```

### REST

```
{
  "tick_size_steps": [
  {
    "above_price": 0.005,
    "tick_size": 0.0005
  }
  ],
  "quote_currency": "BTC",
  "min_trade_amount": 0.1,
  "counter_currency": "USD",
  "settlement_currency": "BTC",
  "block_trade_min_trade_amount": 25,
  "block_trade_commission": 0.0003,
  "option_type": "put",
  "settlement_period": "month",
  "instrument_id": 287296,
  "base_currency": "BTC",
  "block_trade_tick_size": 0.0001,
  "tick_size": 0.0001,
  "contract_size": 1,
  "strike": 62000,
  "is_active": true,
  "expiration_timestamp": 1735286400000,
  "instrument_type": "reversed",
  "taker_commission": 0.0003,
  "maker_commission": 0.0003,
  "instrument_name": "BTC-27DEC24-62000-P",
  "creation_timestamp": 1704753480000,
  "kind": "option",
  "rfq": false,
  "price_index": "btc_usd"
},
```



2022-03-15 (testnet)
Unexpected updates with size 0
NEW
```
header={msg_type_raw="X", msg_type=MARKET_DATA_INCREMENTAL_REFRESH, sender_comp_id="DERIBITSERVER", target_comp_id="ROQ_TRADING", msg_seq_num=1454, sending_time=1647351337728ms},
market_data_incremental_refresh={symbol="BTC-PERPETUAL", md_req_id="roq-12", contract_multiplier=10, put_or_call=UNKNOWN, mark_price=38834.98, open_interest=299006933, no_md_entries=[
  {md_update_action=DELETE, md_entry_type=OFFER, md_entry_px=38836.5, md_entry_size=0, md_entry_date=1647351337727ms, deribit_trade_id="", side=UNKNOWN, order_id="", secondary_order_id="", ord_status=UNKNOWN, deribit_label="", index_price=nan, text="", deribit_liquidation=""},
  {md_update_action=NEW, md_entry_type=OFFER, md_entry_px=38842, md_entry_size=0, md_entry_date=1647351337727ms, deribit_trade_id="", side=UNKNOWN, order_id="", secondary_order_id="", ord_status=UNKNOWN, deribit_label="", index_price=nan, text="", deribit_liquidation=""}
], trade_volume24h=38833176}
```
CHANGE
```
header={msg_type_raw="X", msg_type=MARKET_DATA_INCREMENTAL_REFRESH, sender_comp_id="DERIBITSERVER", target_comp_id="ROQ_TRADING", msg_seq_num=856, sending_time=1647351208363ms},
market_data_incremental_refresh={symbol="BTC-PERPETUAL", md_req_id="roq-12", contract_multiplier=10, put_or_call=UNKNOWN, mark_price=38854.1, open_interest=299001667, no_md_entries=[
{md_update_action=CHANGE, md_entry_type=OFFER, md_entry_px=38865, md_entry_size=0, md_entry_date=1647351208362ms, deribit_trade_id="", side=UNKNOWN, order_id="", secondary_order_id="", ord_status=UNKNOWN, deribit_label="", index_price=nan, text="", deribit_liquidation=""}
], trade_volume24h=38883815}
```

















login="{
"method":"public/auth"
"params":{"grant_type":"client_signature"
"client_id":"5MP40u9h"
"timestamp":"1585292505911"
"nonce":"rvoh0cln98fay4v3z2kob1e50ca6tw7u"
"data":""
"signature":"69df60d2340c4492664f4787aa57b6469ee0e5b61757268593318b92d15579b2"}
"id":"login"}"

access_token="1616828505922.1HkGiwL8.-ekkSJW8OVCGnZgYR0Huz2txShzpz8Np6xZT4X4Wz9TnkAvg1R2_u9md2_IPDYkSATMlRT6Oo8ajDTTO0bHBfNQxJelidqIHjnyKUpBhCuBozx3qRBPfK8-6qCuax3Gi1mKycqPFUTEiLCt9komtKgK_uORr03XcmT1SYf916pDfeS6Slqj9ZD4frSaJBTuE6zxNXhkH-IrpOlx-soq5vN_Gd6MvI5Wiz7KoLR5hj2fEjDXrUBx1HHufWxNlnlDTkRkoEf_Zuy2-HV8gjuNLeHszET8"
expires_in=31536000
refresh_token="1616828505922.1E9uTu5U.oCu5jjTWObJ4U9Bk_aDS0cIaf-7bM9f8Av3SIXu6HSogu4BSI35EKVmJmlTu8_f2L33Lyz51yVyF-Db_QnuHUXZMsi4LEdZY7JdCS8NjbgVA287PyPGwfWwzu9LsQKCmvmQWad5TtrnMVScC1ZmnUtL8Zt-p-UcbXhOGOljHI268Z2aGVLGy1GvggAeudEP93nXadHu5VTEguHX8_7OiP-x2aTQhQNCJC8260zSTgfZM7D12Sx9X1se4vrm8P_6zLNA213mHhYSElt2P522a1bBPbg"
scope="account:read block_trade:read connection mainaccount trade:read_write wallet:read"
state=""
token_type="bearer"


login="{
"method":"public/auth"
"params":{"grant_type":"client_signature"
"client_id":"5MP40u9h"
"timestamp":"1585292596820"
"nonce":"ijdm0e9osr9zm9ff2m2ce3v764seu"
"data":""
"signature":"8f95b8e6816fce9e318265f245e49ddc0b70e20428c3bedf916e1d9917e86217"}
"id":"login"}"

C0327 07:03:16.848638 27530 web_socket.cpp:193] error={code=-32700, message="Parse error", description=""}, id=""
