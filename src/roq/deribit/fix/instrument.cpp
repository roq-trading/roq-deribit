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

Instrument Instrument::parse(
    const core::fix::message_t& message) {
  Instrument result;
  parse(result, message);
  return result;
}

void Instrument::parse(
    Instrument& result,
    const core::fix::message_t& message) {
  auto iter = message.begin();
  result.parse(iter, message.end());
}

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::Instrument::has_field(field);
}
}  // namespace

void Instrument::parse(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  new (this) std::remove_reference<decltype(*this)>::type {};
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::SYMBOL)
    throw core::fix::InvalidField(tag, value);
  core::fix::update(symbol, value);
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    try {
      switch (field) {
        // key
        case core::fix::Field::SYMBOL:
          static_assert(has_field(core::fix::Field::SYMBOL));
          return;  // break
        // standard
        case core::fix::Field::CONTRACT_MULTIPLIER:
          static_assert(has_field(core::fix::Field::CONTRACT_MULTIPLIER));
          core::fix::update(contract_multiplier, value);
          break;
        case core::fix::Field::ISSUE_DATE:
          static_assert(has_field(core::fix::Field::ISSUE_DATE));
          core::fix::update(issue_date, value);
          break;
        case core::fix::Field::MATURITY_DATE:
          static_assert(has_field(core::fix::Field::MATURITY_DATE));
          core::fix::update(maturity_date, value);
          break;
        case core::fix::Field::MATURITY_TIME:
          static_assert(has_field(core::fix::Field::MATURITY_TIME));
          // FIXME(thraneh): TZTimeOnly "08:00:00+00:00"
          // core::fix::update(maturity_time, value);
          break;
        case core::fix::Field::MIN_PRICE_INCREMENT:
          static_assert(has_field(core::fix::Field::MIN_PRICE_INCREMENT));
          core::fix::update(min_price_increment, value);
          break;
        case core::fix::Field::PUT_OR_CALL:
          static_assert(has_field(core::fix::Field::PUT_OR_CALL));
          core::fix::update(put_or_call, value);
          break;
        case core::fix::Field::SECURITY_DESC:
          static_assert(has_field(core::fix::Field::SECURITY_DESC));
          core::fix::update(security_desc, value);
          break;
        case core::fix::Field::SECURITY_TYPE:
          static_assert(has_field(core::fix::Field::SECURITY_TYPE));
          core::fix::update(security_type, value);
          break;
        case core::fix::Field::STRIKE_CURRENCY:
          static_assert(has_field(core::fix::Field::STRIKE_CURRENCY));
          core::fix::update(strike_currency, value);
          break;
        case core::fix::Field::STRIKE_PRICE:
          static_assert(has_field(core::fix::Field::STRIKE_PRICE));
          core::fix::update(strike_price, value);
          break;
        // non-standard
        case core::fix::Field::COMM_CURRENCY:
          static_assert(!has_field(core::fix::Field::COMM_CURRENCY));
          core::fix::update(comm_currency, value);
          break;
        case core::fix::Field::CURRENCY:
          static_assert(!has_field(core::fix::Field::CURRENCY));
          core::fix::update(currency, value);
          break;
        case core::fix::Field::MIN_TRADE_VOL:
          static_assert(!has_field(core::fix::Field::MIN_TRADE_VOL));
          core::fix::update(min_trade_vol, value);
          break;
        case core::fix::Field::SETTL_CURRENCY:
          static_assert(!has_field(core::fix::Field::SETTL_CURRENCY));
          core::fix::update(settl_currency, value);  // FIXME(thraneh): Deribit "[M|W][n]" = n x [months|weeks]
          break;
        case core::fix::Field::SETTL_TYPE:
          static_assert(!has_field(core::fix::Field::SETTL_TYPE));
          core::fix::update(settl_type, value);
          break;
        case core::fix::Field::UNDERLYING_SYMBOL:
          static_assert(!has_field(core::fix::Field::UNDERLYING_SYMBOL));
          core::fix::update(underlying_symbol, value);
          break;
        default:
          if (has_field(field))
            break;
          switch (static_cast<Deribit>(tag)) {
            case Deribit::INSTRUMENT_PRICE_PRECISION:
              core::fix::update(deribit_instrument_price_precision, value);
              break;
            default:
              return;  // unknown field
          }
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
