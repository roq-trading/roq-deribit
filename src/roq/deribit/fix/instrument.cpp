/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/instrument.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/instrument.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::Instrument::has_field(field);
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
      case core::fix::Field::SYMBOL:
        check_field<core::fix::Field::SYMBOL>();
        return false;  // break
      // standard
      case core::fix::Field::CONTRACT_MULTIPLIER:
        check_field<core::fix::Field::CONTRACT_MULTIPLIER>();
        core::fix::update(result.contract_multiplier, value);
        break;
      case core::fix::Field::ISSUE_DATE:
        check_field<core::fix::Field::ISSUE_DATE>();
        core::fix::update(result.issue_date, value);
        break;
      case core::fix::Field::MATURITY_DATE:
        check_field<core::fix::Field::MATURITY_DATE>();
        core::fix::update(result.maturity_date, value);
        break;
      case core::fix::Field::MATURITY_TIME:
        check_field<core::fix::Field::MATURITY_TIME>();
        // FIXME(thraneh): TZTimeOnly "08:00:00+00:00"
        // core::fix::update(result.maturity_time, value);
        break;
      case core::fix::Field::MIN_PRICE_INCREMENT:
        check_field<core::fix::Field::MIN_PRICE_INCREMENT>();
        core::fix::update(result.min_price_increment, value);
        break;
      case core::fix::Field::PUT_OR_CALL:
        check_field<core::fix::Field::PUT_OR_CALL>();
        core::fix::update(result.put_or_call, value);
        break;
      case core::fix::Field::SECURITY_DESC:
        check_field<core::fix::Field::SECURITY_DESC>();
        core::fix::update(result.security_desc, value);
        break;
      case core::fix::Field::SECURITY_TYPE:
        check_field<core::fix::Field::SECURITY_TYPE>();
        core::fix::update(result.security_type, value);
        break;
      case core::fix::Field::STRIKE_CURRENCY:
        check_field<core::fix::Field::STRIKE_CURRENCY>();
        core::fix::update(result.strike_currency, value);
        break;
      case core::fix::Field::STRIKE_PRICE:
        check_field<core::fix::Field::STRIKE_PRICE>();
        core::fix::update(result.strike_price, value);
        break;
      // non-standard
      case core::fix::Field::COMM_CURRENCY:
        non_standard_field<core::fix::Field::COMM_CURRENCY>();
        core::fix::update(result.comm_currency, value);
        break;
      case core::fix::Field::CURRENCY:
        non_standard_field<core::fix::Field::CURRENCY>();
        core::fix::update(result.currency, value);
        break;
      case core::fix::Field::MIN_TRADE_VOL:
        non_standard_field<core::fix::Field::MIN_TRADE_VOL>();
        core::fix::update(result.min_trade_vol, value);
        break;
      case core::fix::Field::SETTL_CURRENCY:
        non_standard_field<core::fix::Field::SETTL_CURRENCY>();
        core::fix::update(result.settl_currency, value);  // FIXME(thraneh): Deribit "[M|W][n]" = n x [months|weeks]
        break;
      case core::fix::Field::SETTL_TYPE:
        non_standard_field<core::fix::Field::SETTL_TYPE>();
        core::fix::update(result.settl_type, value);
        break;
      case core::fix::Field::UNDERLYING_SYMBOL:
        non_standard_field<core::fix::Field::UNDERLYING_SYMBOL>();
        core::fix::update(result.underlying_symbol, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)(
              FMT_STRING("Unexpected tag={} field={}"),
              tag,
              field);
          break;
        }
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
  core::fix::update(
      symbol,
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
