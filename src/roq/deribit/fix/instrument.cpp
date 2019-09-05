/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/instrument.h"

#include "roq/logging.h"

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
  new (&result) std::remove_reference<decltype(result)>::type {};
  auto iter = message.begin();
  result.parse(iter, message.end());
}

void Instrument::parse(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  if (field != core::fix::Field::SYMBOL)
    throw std::runtime_error(
        fmt::format(
            "Expected tag 55 (SYMBOL), got {}",
            (*iter).first));
  update(symbol, value);
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::COMM_CURRENCY:
        update(comm_currency, value);
        break;
      case core::fix::Field::CONTRACT_MULTIPLIER:
        update(contract_multiplier, value);
        break;
      case core::fix::Field::CURRENCY:
        update(currency, value);
        break;
      case core::fix::Field::ISSUE_DATE:
        update(issue_date, value);
        break;
      case core::fix::Field::MATURITY_DATE:
        update(maturity_date, value);
        break;
      case core::fix::Field::MATURITY_TIME:
        // FIXME(thraneh): TZTimeOnly "08:00:00+00:00"
        // update(maturity_time, value);
        break;
      case core::fix::Field::MIN_PRICE_INCREMENT:
        update(min_price_increment, value);
        break;
      case core::fix::Field::MIN_TRADE_VOL:
        update(min_trade_vol, value);
        break;
      case core::fix::Field::PUT_OR_CALL:
        update(put_or_call, value);
        break;
      case core::fix::Field::SECURITY_DESC:
        update(security_desc, value);
        break;
      case core::fix::Field::SECURITY_TYPE:
        update(security_type, value);
        break;
      case core::fix::Field::SETTL_CURRENCY:
        // FIXME(thraneh): Deribit "[M|W][n]" = n x [months|weeks]
        update(settl_currency, value);
        break;
      case core::fix::Field::SETTL_TYPE:
        update(settl_type, value);
        break;
      case core::fix::Field::STRIKE_CURRENCY:
        update(strike_currency, value);
        break;
      case core::fix::Field::STRIKE_PRICE:
        update(strike_price, value);
        break;
      case core::fix::Field::UNDERLYING_SYMBOL:
        update(underlying_symbol, value);
        break;
      // TODO(thraneh): add noop for the remaining FIX spec fields
      default:
        switch (static_cast<Deribit>(tag)) {
          case Deribit::INSTRUMENT_PRICE_PRECISION:
            update(instrument_price_precision, value);
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
