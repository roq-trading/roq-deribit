/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_incremental_refresh.h"

#include "roq/logging.h"

#include "roq/core/fix/market_data_incremental_refresh.h"
#include "roq/core/fix/md_full.h"  // see below
#include "roq/core/fix/md_inc.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
// note! deribit is not standard-compliant !!!
void try_parse_md_full(
    MarketDataIncrementalRefresh::MDIncGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  fprintf(stderr, "MDFull begin\n");
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
    fprintf(stderr, "MDFull tag=%d\n", static_cast<int>(tag));
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
          return;  // key
        case core::fix::Field::MD_UPDATE_ACTION:
          update(result.md_update_action, value);
          break;
        case core::fix::Field::ORDER_ID:
          update(result.order_id, value);
          break;
        case core::fix::Field::SECONDARY_ORDER_ID:
          update(result.secondary_order_id, value);
          break;
        case core::fix::Field::TEXT:  // note! not documented [2019-09-05]
          update(result.text, value);
          break;
        // non-standard
        case core::fix::Field::ORD_STATUS:
          update(result.ord_status, value);
          break;
        case core::fix::Field::PRICE:
          update(result.index_price, value);
          break;
        case core::fix::Field::SIDE:
          update(result.side, value);
          break;
        default:
          if (core::fix::MDInc::has_field(field))
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
      ++iter;
    } catch (std::exception& e) {
      LOG(WARNING) << fmt::format(
          "Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
    ++iter;
  }
  fprintf(stderr, "MDFull done (finished)\n");
}

void parse_md_inc(
    MarketDataIncrementalRefresh::MDIncGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);
  fprintf(stderr, "MDInc begin\n");
  new (&result) std::remove_reference<decltype(result)>::type {};
  // key
  auto& [tag, value] = *iter;
  fprintf(stderr, "key=%d\n", static_cast<int>(tag));
  auto field = core::fix::parse_field(tag);
  static_assert(core::fix::MDInc::key_field == core::fix::Field::MD_UPDATE_ACTION);
  if (field != core::fix::MDInc::key_field) {
    fprintf(stderr, "*** USING FALLBACK ***\n");
    // deribit will occasionally send msg_full messages...
    try_parse_md_full(result, iter, end);
    return;
  }
  fprintf(stderr, "*** NORMAL ***\n");
  update(result.md_update_action, value);
  for (++iter; iter != end;) {
    auto& [tag, value] = *iter;
    fprintf(stderr, "MDInc tag=%d\n", static_cast<int>(tag));
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
          update(result.md_entry_type, value);
          break;
        case core::fix::Field::MD_UPDATE_ACTION:
          return;  // key
        case core::fix::Field::ORDER_ID:
          update(result.order_id, value);
          break;
        case core::fix::Field::SECONDARY_ORDER_ID:
          update(result.secondary_order_id, value);
          break;
        case core::fix::Field::TEXT:  // note! not documented [2019-09-05]
          update(result.text, value);
          break;
        // non-standard
        case core::fix::Field::ORD_STATUS:
          update(result.ord_status, value);
          break;
        case core::fix::Field::PRICE:
          update(result.index_price, value);
          break;
        case core::fix::Field::SIDE:
          update(result.side, value);
          break;
        default:
          if (core::fix::MDInc::has_field(field))
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
      ++iter;
    } catch (std::exception& e) {
      LOG(WARNING) << fmt::format(
          "Can't parse tag={} value=\"{}\"", tag, value);
      throw;
    }
    ++iter;
  }
  fprintf(stderr, "MDInc done (finished)\n");
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
  while (iter != end) {
    fprintf(stderr, "MarketIncrementalRefresh begin\n");
    auto& [tag, value] = *iter;
    fprintf(stderr, "MarketIncrementalRefresh tag=%d\n", static_cast<int>(tag));
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        // standard
        case core::fix::Field::MD_REQ_ID:
          update(md_req_id, value);
          break;
        case core::fix::Field::NO_MD_ENTRIES: {
          fprintf(stderr, "MDInc begin\n");
          auto length = core::charconv::from_string<uint32_t>(value);
          ++iter;
          Array array(buffer_, md_inc_grp);
          for (uint32_t i = 0; i < length; ++i) {
            fprintf(stderr, "MDInc %d of %d\n", static_cast<int>(i), static_cast<int>(length));
            if (iter == end)
              throw std::runtime_error(
                  fmt::format("Unable to parse MDInc {} of {}",
                    i, length));
            auto& item = array.next();
            parse_md_inc(item, iter, end);
            ++array;
          }
          fprintf(stderr, "MDInc done (%d)\n", iter == end ? 1 : 0);
          if (md_inc_grp.length != length)
            throw std::runtime_error("Wrong length");
          continue;
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
        default:
          if (core::fix::MarketDataIncrementalRefresh::has_field(field))
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
              LOG(WARNING) <<
                fmt::format(
                    "Unknown field: tag={} field={} value=\"{}\"",
                    tag, field, value);
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
