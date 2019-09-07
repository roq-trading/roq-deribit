/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_snapshot_full_refresh.h"

#include "roq/logging.h"

#include "roq/core/fix/market_data_snapshot_full_refresh.h"
#include "roq/core/fix/md_full.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
void parse_md_full(
    MarketDataSnapshotFullRefresh::MDFullGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);
  new (&result) std::remove_reference<decltype(result)>::type {};
  // key
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  static_assert(core::fix::MDFull::key_field == core::fix::Field::MD_ENTRY_TYPE);
  if (field != core::fix::MDFull::key_field) {
    throw std::runtime_error(
        fmt::format(
            "Expected tag={} ({}), got tag={} ({})",
            static_cast<int>(core::fix::MDFull::key_field),
            core::fix::MDFull::key_field,
            tag,
            field));
  }
  update(result.md_entry_type, value);
  for (++iter; iter != end;) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        // standard
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
          // key
          return;
        case core::fix::Field::SECONDARY_ORDER_ID:
          update(result.secondary_order_id, value);
          break;
        case core::fix::Field::TEXT:
          update(result.text, value);
          break;
        // non-standard
        case core::fix::Field::MD_UPDATE_ACTION:
          return;
        case core::fix::Field::ORD_STATUS:
          update(result.ord_status, value);
          break;
        case core::fix::Field::SIDE:
          update(result.side, value);
          break;
        default:
          if (core::fix::MDFull::has_field(field))
            break;
          // deribit specific
          switch (static_cast<Deribit>(tag)) {
            case Deribit::LABEL:
              update(result.deribit_label, value);
              break;
            case Deribit::LIQUIDATION:
              update(result.deribit_liquidation, value);
              break;
            case Deribit::TRADE_ID:
              update(result.deribit_trade_id, value);
              break;
            default:
              LOG(WARNING) <<
                fmt::format(
                    "Unknown field: tag={} field={} value=\"{}\"",
                    tag, field, value);
              return;
          }
      }
    } catch (std::exception& e) {
      LOG(WARNING) <<
        fmt::format("Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
    ++iter;
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
  while (iter != end) {
    auto [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        // standard
        case core::fix::Field::MD_REQ_ID:
          update(md_req_id, value);
          break;
        case core::fix::Field::NO_MD_ENTRIES: {
          auto length = core::charconv::from_string<uint32_t>(value);
          ++iter;
          Array array(buffer_, md_full_grp);
          for (uint32_t i = 0; i < length; ++i) {
            if (iter == end)
              throw std::runtime_error(
                  fmt::format("Unable to parse MDFull {} of {}",
                    i, length));
            auto& item = array.next();
            parse_md_full(item, iter, end);
            ++array;
          }
          if (md_full_grp.length != length)
            throw std::runtime_error("Wrong length");
          continue;  // iterator has already been advanced
        }
        // non-standard
        case core::fix::Field::CONTRACT_MULTIPLIER:
          update(contract_multiplier, value);
          break;
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
          if (core::fix::MarketDataSnapshotFullRefresh::has_field(field))
            break;
          // deribit specific
          switch (static_cast<Deribit>(tag)) {
            case Deribit::MARK_PRICE:
              update(deribit_mark_price, value);
              break;
            case Deribit::TRADE_VOLUME_24H:
              update(deribit_trade_volume_24h, value);
              break;
            default:
              LOG(WARNING) << fmt::format(
                  "Unknown field: tag={} field={} value=\"{}\"",
                  tag,
                  field,
                  value);
          }
      }
    } catch (std::exception& e) {
      LOG(WARNING) <<
        fmt::format("Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
