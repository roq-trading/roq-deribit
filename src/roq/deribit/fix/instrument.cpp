/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/instrument.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/instrument.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::Instrument::has_field(field);
}

bool update(
    auto& result,
    const auto& tag,
    const auto& field,
    const auto& value) {
  try {
    switch (field) {
      // key
      case core::fix::Field::SYMBOL:
        static_assert(has_field(core::fix::Field::SYMBOL));
        return false;  // break
      // standard
      case core::fix::Field::CONTRACT_MULTIPLIER:
        static_assert(has_field(core::fix::Field::CONTRACT_MULTIPLIER));
        core::fix::update(result.contract_multiplier, value);
        break;
      case core::fix::Field::ISSUE_DATE:
        static_assert(has_field(core::fix::Field::ISSUE_DATE));
        core::fix::update(result.issue_date, value);
        break;
      case core::fix::Field::MATURITY_DATE:
        static_assert(has_field(core::fix::Field::MATURITY_DATE));
        core::fix::update(result.maturity_date, value);
        break;
      case core::fix::Field::MATURITY_TIME:
        static_assert(has_field(core::fix::Field::MATURITY_TIME));
        // FIXME(thraneh): TZTimeOnly "08:00:00+00:00"
        // core::fix::update(result.maturity_time, value);
        break;
      case core::fix::Field::MIN_PRICE_INCREMENT:
        static_assert(has_field(core::fix::Field::MIN_PRICE_INCREMENT));
        core::fix::update(result.min_price_increment, value);
        break;
      case core::fix::Field::PUT_OR_CALL:
        static_assert(has_field(core::fix::Field::PUT_OR_CALL));
        core::fix::update(result.put_or_call, value);
        break;
      case core::fix::Field::SECURITY_DESC:
        static_assert(has_field(core::fix::Field::SECURITY_DESC));
        core::fix::update(result.security_desc, value);
        break;
      case core::fix::Field::SECURITY_TYPE:
        static_assert(has_field(core::fix::Field::SECURITY_TYPE));
        core::fix::update(result.security_type, value);
        break;
      case core::fix::Field::STRIKE_CURRENCY:
        static_assert(has_field(core::fix::Field::STRIKE_CURRENCY));
        core::fix::update(result.strike_currency, value);
        break;
      case core::fix::Field::STRIKE_PRICE:
        static_assert(has_field(core::fix::Field::STRIKE_PRICE));
        core::fix::update(result.strike_price, value);
        break;
      // non-standard
      case core::fix::Field::COMM_CURRENCY:
        static_assert(!has_field(core::fix::Field::COMM_CURRENCY));
        core::fix::update(result.comm_currency, value);
        break;
      case core::fix::Field::CURRENCY:
        static_assert(!has_field(core::fix::Field::CURRENCY));
        core::fix::update(result.currency, value);
        break;
      case core::fix::Field::MIN_TRADE_VOL:
        static_assert(!has_field(core::fix::Field::MIN_TRADE_VOL));
        core::fix::update(result.min_trade_vol, value);
        break;
      case core::fix::Field::SETTL_CURRENCY:
        static_assert(!has_field(core::fix::Field::SETTL_CURRENCY));
        core::fix::update(result.settl_currency, value);  // FIXME(thraneh): Deribit "[M|W][n]" = n x [months|weeks]
        break;
      case core::fix::Field::SETTL_TYPE:
        static_assert(!has_field(core::fix::Field::SETTL_TYPE));
        core::fix::update(result.settl_type, value);
        break;
      case core::fix::Field::UNDERLYING_SYMBOL:
        static_assert(!has_field(core::fix::Field::UNDERLYING_SYMBOL));
        core::fix::update(result.underlying_symbol, value);
        break;
      default:
        if (has_field(field))
          break;
        switch (static_cast<Deribit>(tag)) {
          case Deribit::INSTRUMENT_PRICE_PRECISION:
            core::fix::update(result.deribit_instrument_price_precision, value);
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

Instrument::Instrument(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);

  // XXX can we move this to Array?
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::SYMBOL)
    throw core::fix::InvalidField(tag, value);
  core::fix::update(symbol, value);

  // remaining fields
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    if (update(*this, tag, field, value) == false)
      return;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
