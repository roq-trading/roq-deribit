/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/position_qty.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/position_qty.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::PositionQty::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

bool update_field(
    auto& result,
    const auto& tag,
    const auto& field,
    const auto& value) {
  try {
    switch (field) {
      // key
      case core::fix::Field::POS_TYPE:
        check_field<core::fix::Field::POS_TYPE>();
        return false;  // break
      // standard
      case core::fix::Field::LONG_QTY:
        check_field<core::fix::Field::LONG_QTY>();
        core::fix::update(result.long_qty, value);
        break;
      case core::fix::Field::SHORT_QTY:
        check_field<core::fix::Field::SHORT_QTY>();
        core::fix::update(result.short_qty, value);
        break;
      // non-standard
      case core::fix::Field::CONTRACT_MULTIPLIER:
        non_standard_field<core::fix::Field::CONTRACT_MULTIPLIER>();
        core::fix::update(result.contract_multiplier, value);
        break;
      case core::fix::Field::QTY_TYPE:
        non_standard_field<core::fix::Field::QTY_TYPE>();
        core::fix::update(result.qty_type, value);
        break;
      case core::fix::Field::RAW_DATA_LENGTH:
        non_standard_field<core::fix::Field::RAW_DATA_LENGTH>();
        // nothing to do...
        break;
      case core::fix::Field::RAW_DATA:
        non_standard_field<core::fix::Field::RAW_DATA>();
        core::fix::update(result.raw_data, value);
        break;
      case core::fix::Field::SETTL_PRICE:
        non_standard_field<core::fix::Field::SETTL_PRICE>();
        core::fix::update(result.settl_price, value);
        break;
      case core::fix::Field::SIDE:
        non_standard_field<core::fix::Field::SIDE>();
        core::fix::update(result.side, value);
        break;
      case core::fix::Field::SYMBOL:
        non_standard_field<core::fix::Field::SYMBOL>();
        core::fix::update(result.symbol, value);
        break;
      case core::fix::Field::UNDERLYING_PRICE:
        non_standard_field<core::fix::Field::UNDERLYING_PRICE>();
        core::fix::update(result.underlying_price, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
          break;
        }
        switch (static_cast<Deribit>(tag)) {
          case Deribit::LIQUIDATION_PRICE:
            core::fix::update(result.deribit_liquidation_price, value);
            break;
          case Deribit::SIZE_IN_CURRENCY:
            core::fix::update(result.deribit_size_in_currency, value);
            break;
          default:
            return false;  // unknown field
        }
    }
    return true;
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

PositionQty::PositionQty(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);

  // XXX can we move this to Array?
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::POS_TYPE)
    throw core::fix::InvalidField(tag, value);
  core::fix::update(
      pos_type,
      value);

  // remaining fields
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    if (update_field(
          *this,
          tag,
          field,
          value) == false)
      break;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
