/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_incremental_refresh.h"

#include "roq/logging.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
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

void parse_md_inc(
    MarketDataIncrementalRefresh::MDIncGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  fprintf(stderr, "md_inc_grp\n");
  new (&result) std::remove_reference<decltype(result)>::type {};
  auto& [tag, value] = *iter;
  fprintf(stderr, "lead: tag=%d\n",
      static_cast<int>(tag));
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::MD_UPDATE_ACTION)
    throw std::runtime_error(
        fmt::format(
            "Expected tag 279 (MD_UPDATE_ACTION), got {}",
            (*iter).first));
  update(result.md_update_action, value);
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    fprintf(stderr, "iter: tag=%d\n",
        static_cast<int>(tag));
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::PRICE:
        update(result.index_price, value);
        break;
      case core::fix::Field::MD_ENTRY_DATE:
        update(result.md_entry_date, value);
        break;
      case core::fix::Field::MD_ENTRY_PX:
        update(result.md_entry_px, value);
        break;
      case core::fix::Field::MD_ENTRY_SIZE:
        update(result.md_entry_size, value);
        break;
      case core::fix::Field::MD_ENTRY_TYPE:
        update(result.md_entry_type, value);
        break;
      case core::fix::Field::ORD_STATUS:
        update(result.ord_status, value);
        break;
      case core::fix::Field::ORDER_ID:
        update(result.order_id, value);
        break;
      case core::fix::Field::SECONDARY_ORDER_ID:
        update(result.secondary_order_id, value);
        break;
      case core::fix::Field::SIDE:
        update(result.side, value);
        break;
      case core::fix::Field::TEXT:  // note! not documented as of 2019-09-05
        update(result.text, value);
        break;
      // TODO(thraneh): add noop for the remaining FIX spec fields
      default:
        switch (static_cast<Deribit>(tag)) {
          case Deribit::LABEL:
            update(result.deribit_label, value);
            break;
          case Deribit::TRADE_ID:
            update(result.deribit_trade_id, value);
            break;
          default:
            return;
        }
    }
  }
}

}  // namespace

MarketDataIncrementalRefresh MarketDataIncrementalRefresh::parse(
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  MarketDataIncrementalRefresh result;
  parse(result, message, buffer);
  return result;
}

void MarketDataIncrementalRefresh::parse(
    MarketDataIncrementalRefresh& result,
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

void MarketDataIncrementalRefresh::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    std::vector<std::byte>& buffer) {
  Buffer buffer_(buffer);
  for (; iter != end;) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::CONTRACT_MULTIPLIER:
        update(contract_multiplier, value);
        break;
      case core::fix::Field::MD_REQ_ID:
        update(md_req_id, value);
        break;
      case core::fix::Field::NO_MD_ENTRIES: {
        auto length = core::charconv::from_string<uint32_t>(value);
        ++iter;
        Array array(buffer_, md_inc_grp);
        for (uint32_t i = 0; i < length; ++i) {
          auto& item = array.next();
          parse_md_inc(item, iter, end);
          ++array;
        }
        continue;
      }
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
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
