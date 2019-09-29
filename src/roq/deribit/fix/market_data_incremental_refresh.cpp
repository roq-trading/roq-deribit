/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_incremental_refresh.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/market_data_incremental_refresh.h"
#include "roq/core/fix/md_inc.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
bool update_md_inc(
    auto& result,
    const auto& tag,
    const auto& field,
    const auto& value) {
  try {
    switch (field) {
      // key
      case core::fix::Field::MD_UPDATE_ACTION:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::MD_UPDATE_ACTION));
        return false;  // break
      // standard
      case core::fix::Field::MD_ENTRY_DATE:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::MD_ENTRY_DATE));
        core::fix::update(result.md_entry_date, value);
        break;
      case core::fix::Field::MD_ENTRY_PX:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::MD_ENTRY_PX));
        core::fix::update(result.md_entry_px, value);
        break;
      case core::fix::Field::MD_ENTRY_SIZE:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::MD_ENTRY_SIZE));
        core::fix::update(result.md_entry_size, value);
        break;
      case core::fix::Field::MD_ENTRY_TYPE:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::MD_ENTRY_TYPE));
        core::fix::update(result.md_entry_type, value);
        break;
      case core::fix::Field::ORDER_ID:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::ORDER_ID));
        core::fix::update(result.order_id, value);
        break;
      case core::fix::Field::SECONDARY_ORDER_ID:
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::SECONDARY_ORDER_ID));
        core::fix::update(result.secondary_order_id, value);
        break;
      case core::fix::Field::TEXT:  // note! not documented [2019-09-05]
        static_assert(core::fix::MDIncGrp::has_field(core::fix::Field::TEXT));
        core::fix::update(result.text, value);
        break;
      // non-standard
      case core::fix::Field::ORD_STATUS:
        static_assert(!core::fix::MDIncGrp::has_field(core::fix::Field::ORD_STATUS));
        core::fix::update(result.ord_status, value);
        break;
      case core::fix::Field::PRICE:
        static_assert(!core::fix::MDIncGrp::has_field(core::fix::Field::PRICE));
        core::fix::update(result.index_price, value);
        break;
      case core::fix::Field::SIDE:
        static_assert(!core::fix::MDIncGrp::has_field(core::fix::Field::SIDE));
        core::fix::update(result.side, value);
        break;
      default:
        if (core::fix::MDInc::has_field(field))
          break;
        // deribit specific
        switch (static_cast<Deribit>(tag)) {
          case Deribit::LABEL:
            core::fix::update(result.deribit_label, value);
            break;
          case Deribit::LIQUIDATION:
            core::fix::update(result.deribit_liquidation, value);
            break;
          case Deribit::TRADE_ID:
            core::fix::update(result.deribit_trade_id, value);
            break;
          default:
            return false;
        }
    }
    return true;
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(
        "MarketDataIncrementalRefresh|MDIncGrp: "
        "Parse error: "
        "field={}, value=\"{}\", what=\"{}\"",
        tag, value, e.what());
  }
}

void parse_md_inc(
    MarketDataIncrementalRefresh::MDIncGrp& result,
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);
  new (&result) std::remove_reference<decltype(result)>::type {};
  // key
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  static_assert(core::fix::MDInc::key_field == core::fix::Field::MD_UPDATE_ACTION);
  if (field != core::fix::MDInc::key_field)
    throw core::fix::InvalidField(
        "MarketDataIncrementalRefresh|MDIncGrp: "
        "Unexpected first field={}", tag);
  core::fix::update(result.md_update_action, value);
  // remaining fields
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    if (update_md_inc(result, tag, field, value) == false)
      return;
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
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        // standard
        case core::fix::Field::CONTRACT_MULTIPLIER:
          static_assert(core::fix::MarketDataIncrementalRefresh::has_field(core::fix::Field::CONTRACT_MULTIPLIER));
          core::fix::update(contract_multiplier, value);
          break;
        case core::fix::Field::NO_MD_ENTRIES: {
          static_assert(core::fix::MarketDataIncrementalRefresh::has_field(core::fix::Field::NO_MD_ENTRIES));
          auto length = core::charconv::from_string<uint32_t>(value);
          ++iter;
          Array array(buffer_, md_inc_grp);
          for (uint32_t i = 0; i < length; ++i) {
            if (iter == end)
              throw core::fix::UnexpectedEndOfMessage(
                  "MarketDataIncrementalRefresh|MDIncGrp");
            auto& item = array.next();
            parse_md_inc(item, iter, end);
            ++array;
          }
          if (md_inc_grp.length != length)
            throw core::fix::InvalidGroupLength(
                "MarketDataIncrementalRefresh|MDIncGrp: "
                "Invalid group length: parsed={}, expected={}",
                md_inc_grp.length, length);
          continue;
        }
        case core::fix::Field::MD_REQ_ID:
          static_assert(core::fix::MarketDataIncrementalRefresh::has_field(core::fix::Field::MD_REQ_ID));
          core::fix::update(md_req_id, value);
          break;
        case core::fix::Field::SYMBOL:
          static_assert(core::fix::MarketDataIncrementalRefresh::has_field(core::fix::Field::SYMBOL));
          core::fix::update(symbol, value);
          break;
        // non-standard
        case core::fix::Field::OPEN_INTEREST:
          static_assert(!core::fix::MarketDataIncrementalRefresh::has_field(core::fix::Field::OPEN_INTEREST));
          core::fix::update(open_interest, value);
          break;
        default:
          if (core::fix::MarketDataIncrementalRefresh::has_field(field))
            break;
          // deribit specific
          switch (static_cast<Deribit>(tag)) {
            case Deribit::MARK_PRICE:
              core::fix::update(deribit_mark_price, value);
              break;
            case Deribit::TRADE_VOLUME_24H:
              core::fix::update(deribit_trade_volume_24h, value);
              break;
            default:
              throw core::fix::InvalidField(
                  "MarketDataIncrementalRefresh: "
                  "Unexpected field={}", tag);
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "MarketDataIncrementalRefresh: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
