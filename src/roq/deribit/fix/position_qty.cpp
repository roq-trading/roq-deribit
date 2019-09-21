/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/position_qty.h"

#include <fmt/format.h>

#include <stdexcept>

#include "roq/core/fix/position_qty.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

PositionQty PositionQty::parse(
    const core::fix::message_t& message) {
  PositionQty result;
  parse(result, message);
  return result;
}

void PositionQty::parse(
    PositionQty& result,
    const core::fix::message_t& message) {
  auto iter = message.begin();
  result.parse(iter, message.end());
}

void PositionQty::parse(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  new (this) std::remove_reference<decltype(*this)>::type {};
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::POS_TYPE)
    throw std::runtime_error(
        fmt::format(
            "Expected tag POS_TYPE, got {}",
            (*iter).first));
  core::fix::update(pos_type, value);
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::POS_TYPE:
        static_assert(core::fix::PositionQty::has_field(core::fix::Field::POS_TYPE));
        return;
      case core::fix::Field::LONG_QTY:
        static_assert(core::fix::PositionQty::has_field(core::fix::Field::LONG_QTY));
        core::fix::update(long_qty, value);
        break;
      case core::fix::Field::SHORT_QTY:
        static_assert(core::fix::PositionQty::has_field(core::fix::Field::SHORT_QTY));
        core::fix::update(short_qty, value);
        break;
      // non-standard
      case core::fix::Field::CONTRACT_MULTIPLIER:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::CONTRACT_MULTIPLIER));
        core::fix::update(contract_multiplier, value);
        break;
      case core::fix::Field::QTY_TYPE:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::QTY_TYPE));
        core::fix::update(qty_type, value);
        break;
      case core::fix::Field::RAW_DATA_LENGTH:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::RAW_DATA_LENGTH));
        // nothing to do...
        break;
      case core::fix::Field::RAW_DATA:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::RAW_DATA));
        core::fix::update(raw_data, value);
        break;
      case core::fix::Field::SETTL_PRICE:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::SETTL_PRICE));
        core::fix::update(settl_price, value);
        break;
      case core::fix::Field::SIDE:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::SIDE));
        core::fix::update(side, value);
        break;
      case core::fix::Field::SYMBOL:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::SYMBOL));
        core::fix::update(symbol, value);
        break;
      case core::fix::Field::UNDERLYING_PRICE:
        static_assert(!core::fix::PositionQty::has_field(core::fix::Field::UNDERLYING_PRICE));
        core::fix::update(underlying_price, value);
        break;
      default:
        if (core::fix::PositionQty::has_field(field))
          break;
        switch (static_cast<Deribit>(tag)) {
          case Deribit::LIQUIDATION_PRICE:
            core::fix::update(deribit_liquidation_price, value);
            break;
          case Deribit::SIZE_IN_CURRENCY:
            core::fix::update(deribit_size_in_currency, value);
            break;
          default:
            return;
        }
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
