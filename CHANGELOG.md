# Change Log

All notable changes will be documented in this file.

## Head

### Changed

* `TopOfBook` now convert `Quote.best_bid_amount` and `Quote.best_ask_amount`
  to number of contracts

## 0.7.2 &ndash; 2021-06-20

## 0.7.1 &ndash; 2021-05-30

## 0.7.0 &ndash; 2021-04-15

### Added

* Multi-account support

### Changed

* Streams to support load-balancing
* Using web-socket for funds and positions

## 0.6.1 &ndash; 2021-02-19

## 0.6.0 &ndash; 2021-02-02

## 0.5.0 &ndash; 2020-12-04

## 0.4.5 &ndash; 2020-11-09

## 0.4.4 &ndash; 2020-09-20

### Changed

* Default config excludes `"USDT-.*"` due to missing market data on testnet


## 0.4.3 &ndash; 2020-09-02

## 0.4.2 &ndash; 2020-07-27

### Removed

* Automake support

## 0.4.1 &ndash; 2020-07-17

## 0.4.0 &ndash; 2020-06-30

### Added

* `fix::ExecutionReport::SecondaryExecID` (tag 527)
* `json::Instrument::block_trade_commission`

## 0.3.9 &ndash; 2020-06-09

## 0.3.8 &ndash; 2020-06-06

## 0.3.7 &ndash; 2020-05-27

### Added

* `SessionStatistics` now used to propagate the index value
  (`index_value`), funding rate (`margin_rate`) and mark price
  (`pre_settlement_price`) from `fix::MarketDataIncrementalRefresh`

## 0.3.6 &ndash; 2020-05-02

## 0.3.5 &ndash; 2020-04-22

### Added

* `fix::SecurityStatus` and `fix::SecurityStatusRequst` (but not yet
   using)

### Changed

* `fix::Reject` "connection too slow" will now cause connection reset

## 0.3.4 &ndash; 2020-04-08

### Added

* `OrderUpdate::execution_instruction` support
* `json::Ticker::delivery_price` parsing
* Web-Socket support (for download)

### Removed

* `OrderUpdate::commissions`

### Changed

* New download state management

## 0.3.3 &ndash; 2020-03-04
