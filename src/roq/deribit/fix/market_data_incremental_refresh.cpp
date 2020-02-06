/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_incremental_refresh.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/market_data_incremental_refresh.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::MarketDataIncrementalRefresh::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

void update(
    auto& result,
    auto&& iter,
    const auto& end,
    auto& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    try {
      auto field = core::fix::parse_field(tag);
      switch (field) {
        // standard
        case core::fix::Field::CONTRACT_MULTIPLIER:
          check_field<core::fix::Field::CONTRACT_MULTIPLIER>();
          core::fix::update(result.contract_multiplier, value);
          break;
        case core::fix::Field::NO_MD_ENTRIES: {
          check_field<core::fix::Field::NO_MD_ENTRIES>();
          result.md_inc_grp =
            core::fix::Array<decltype(result.md_inc_grp)>::create(
                buffer,
                iter,
                end);
          continue;  // note!
        }
        case core::fix::Field::MD_REQ_ID:
          check_field<core::fix::Field::MD_REQ_ID>();
          core::fix::update(result.md_req_id, value);
          break;
        case core::fix::Field::PUT_OR_CALL:
          check_field<core::fix::Field::PUT_OR_CALL>();
          core::fix::update(result.put_or_call, value);
          break;
        case core::fix::Field::SYMBOL:
          check_field<core::fix::Field::SYMBOL>();
          core::fix::update(result.symbol, value);
          break;
        // non-standard
        case core::fix::Field::OPEN_INTEREST:
          non_standard_field<core::fix::Field::OPEN_INTEREST>();
          core::fix::update(result.open_interest, value);
          break;
        default:
          if (has_field(field)) {
            DLOG(FATAL)(
                FMT_STRING("Unexpected tag={} field={}"),
                tag,
                field);
            break;
          }
          // deribit specific
          switch (static_cast<Deribit>(tag)) {
            case Deribit::MARK_PRICE:
              core::fix::update(result.deribit_mark_price, value);
              break;
            case Deribit::TRADE_VOLUME_24H:
              core::fix::update(result.deribit_trade_volume_24h, value);
              break;
            default:
              DLOG(FATAL)(
                  FMT_STRING("Unknown tag={} field={}"),
                  tag,
                  field);
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
}  // namespace

MarketDataIncrementalRefresh MarketDataIncrementalRefresh::create(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  MarketDataIncrementalRefresh result;
  update(
      result,
      message.begin(),
      message.end(),
      buffer);
  return result;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
