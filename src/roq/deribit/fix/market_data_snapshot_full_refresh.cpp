/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"

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
=>279 MDUpdateAction  Char  No  Type of Market Data update action. 0 = New, 1 = Change, 2 = Delete
=>269 MDEntryType int No  0 = Bid (Bid side of the order book), 1 = Offer (Ask side of the order book), 2 = Trade (in case of request for info about recent trades)
=>270 MDEntryPx Price No  Price of an entry
=>271 MDEntrySize Qty No  Size of an entry
=>272 MDEntryDate UTCTimestamp  No  The timestamp for trade
=>100009  DeribitTradeId  int No  Id of the trade, in case of the request for trades
=>54  Side  int No  Side of trade (1 = Buy, 2 = Sell)
=>58  Text  String  No  The trade sequence number
=>198 SecondaryOrderId  String  No  For trade – matching order id
=>39  OrdStatus int No  For trade – order status (0 = New, 1 = Partially filled, 2 = Filled, 4 = Cancelled)
=>100010  DeribitLabel  String  No  User defined 32 character label of the order, in case of the request for trades
*/
void parse_md_full(
    MarketDataSnapshotFullRefresh::MDFullGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  fprintf(stderr, "md_full_grp\n");
  new (&result) std::remove_reference<decltype(result)>::type {};
  auto& [tag, value] = *iter;
  fprintf(stderr, "lead: tag=%d\n",
      static_cast<int>(tag));
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::MD_ENTRY_TYPE)
    throw std::runtime_error(
        fmt::format(
            "Expected tag 269 (MD_ENTRY_TYPE), got {}",
            (*iter).first));
  update(result.md_entry_type, value);
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    fprintf(stderr, "iter: tag=%d value=\"%.*s\"\n",
        static_cast<int>(tag),
        static_cast<int>(value.length()),
        value.data());
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        case core::fix::Field::MD_ENTRY_DATE:
          update(result.md_entry_date, value);
          break;
        case core::fix::Field::MD_ENTRY_PX:
          update(result.md_entry_px, value);
          break;
        case core::fix::Field::MD_ENTRY_SIZE:
          update(result.md_entry_size, value);
          break;
        case core::fix::Field::ORD_STATUS:
          update(result.ord_status, value);
          break;
        case core::fix::Field::SECONDARY_ORDER_ID:
          update(result.secondary_order_id, value);
          break;
        case core::fix::Field::SIDE:
          update(result.side, value);
          break;
        case core::fix::Field::TEXT:
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
              fprintf(stderr, "stop: tag=%d\n",
                  static_cast<int>(tag));
              return;
          }
      }
    } catch (std::exception& e) {
      fprintf(stderr, "fail: tag=%d value=\"%.*s\"\n",
          static_cast<int>(tag),
          static_cast<int>(value.length()),
          value.data());
      throw;
    }
  }
}
}  // namespace

MarketDataSnapshotFullRefresh MarketDataSnapshotFullRefresh::parse(
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  MarketDataSnapshotFullRefresh result;
  parse(result, message, buffer);
  return result;
}

void MarketDataSnapshotFullRefresh::parse(
    MarketDataSnapshotFullRefresh& result,
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

void MarketDataSnapshotFullRefresh::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    std::vector<std::byte>& buffer) {
  Buffer buffer_(buffer);
  for (; iter != end; ++iter) {
    auto [tag, value] = *iter;
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
        Array array(buffer_, md_full_grp);
        for (uint32_t i = 0; i < length; ++i) {
          auto& item = array.next();
          parse_md_full(item, iter, end);
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
      case core::fix::Field::UNDERLYING_PX:
        update(underlying_px, value);
        break;
      case core::fix::Field::UNDERLYING_SYMBOL:
        update(underlying_symbol, value);
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
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
