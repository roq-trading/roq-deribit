/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_incremental_refresh.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/market_data_incremental_refresh.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

MarketDataIncrementalRefresh MarketDataIncrementalRefresh::parse(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  MarketDataIncrementalRefresh result;  // FIXME(thraneh): 2x init?
  parse(result, message, buffer);
  return result;
}

void MarketDataIncrementalRefresh::parse(
    MarketDataIncrementalRefresh& result,
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::MarketDataIncrementalRefresh::has_field(field);
}
}  // namespace

void MarketDataIncrementalRefresh::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    core::fix::Buffer& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        // standard
        case core::fix::Field::CONTRACT_MULTIPLIER:
          static_assert(has_field(core::fix::Field::CONTRACT_MULTIPLIER));
          core::fix::update(contract_multiplier, value);
          break;
        case core::fix::Field::NO_MD_ENTRIES: {
          static_assert(has_field(core::fix::Field::NO_MD_ENTRIES));
          md_inc_grp = core::fix::Array<decltype(md_inc_grp)>::parse(
              buffer,
              iter,
              end);
          continue;  // note!
        }
        case core::fix::Field::MD_REQ_ID:
          static_assert(has_field(core::fix::Field::MD_REQ_ID));
          core::fix::update(md_req_id, value);
          break;
        case core::fix::Field::SYMBOL:
          static_assert(has_field(core::fix::Field::SYMBOL));
          core::fix::update(symbol, value);
          break;
        // non-standard
        case core::fix::Field::OPEN_INTEREST:
          static_assert(!has_field(core::fix::Field::OPEN_INTEREST));
          core::fix::update(open_interest, value);
          break;
        default:
          if (has_field(field))
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
              throw core::fix::InvalidField(tag, value);
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
