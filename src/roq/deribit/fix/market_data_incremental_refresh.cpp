/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_incremental_refresh.h"

#include "roq/logging.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

/*
268 NoMDEntries int Yes Repeating group . Specifies the number of entries in the group.
=>279 MDUpdateAction  char  No  Type of Market Data update action. Valid values: 0 = New, 1 = Change, 2 = Delete
=>269 MDEntryType int No  0 = Bid (Bid side of the order book), 1 = Offer (Ask side of the order book), 2 = Trade (in case of request for info about recent trades)
=>270 MDEntryPx Price No  Price of an entry
=>271 MDEntrySize Qty No  Size of an entry
=>272 MDEntryDate String  UTCTimestamp  The timestamp for trade
=>100009  DeribitTradeId  int No  Id of the trade, in case of the request for trades
=>54  Side  int No  Side of trade (1 = Buy, 2 = Sell)
=>37  OrderId String  No  For trade – order id
=>198 SecondaryOrderId  String  No  For trade – matching order id
=>39  OrdStatus int No  For trade – order status (0 = New, 1 = Partially filled, 2 = Filled, 4 = Cancelled)
=>100010  DeribitLabel  String  No  User defined 32 character label of the order, in case of the request for trades
=>44  IndexPrice  Price No  For trades, this is the index price at the trade moment (Deribit index).
*/

MarketDataIncrementalRefresh MarketDataIncrementalRefresh::parse(
    const core::fix::message_t& message) {
  MarketDataIncrementalRefresh result;
  parse(result, message);
  return result;
}

void MarketDataIncrementalRefresh::parse(
    MarketDataIncrementalRefresh& result,
    const core::fix::message_t& message) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end());
}

void MarketDataIncrementalRefresh::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end) {
  for (; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::CONTRACT_MULTIPLIER:
        update(contract_multiplier, value);
        break;
      case core::fix::Field::MD_REQ_ID:
        update(md_req_id, value);
        break;
      case core::fix::Field::NO_MD_ENTRIES:
        // Buffer buffer(buffer_);
        // Array bids(buffer, result.bids);
        // auto& bid = bids.next();
        continue;  // different
      case core::fix::Field::OPEN_INTEREST:
        update(open_interest, value);
        break;
      case core::fix::Field::SYMBOL:
        update(symbol, value);
        break;
      default:
        switch (static_cast<Deribit>(tag)) {
          case Deribit::MARK_PRICE:
            update(mark_price, value);
            break;
          case Deribit::TRADE_VOLUME_24H:
            update(trade_volume_24h, value);
            break;
          default:
            LOG(WARNING) << fmt::format(
                "Unknown field: tag={} field={} value=\"{}\"",
                tag,
                field,
                value);
        }
    }
  }
  ++iter;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
