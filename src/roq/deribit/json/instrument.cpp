/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/json/instrument.h"

#include "roq/core/json/parser.h"

#include "roq/deribit/json/utils.h"

namespace roq {
namespace deribit {
namespace json {

namespace {
enum class Field {
  UNKNOWN,
  BASE_CURRENCY,
  CONTRACT_SIZE,
  CREATION_TIMESTAMP,
  EXPIRATION_TIMESTAMP,
  INSTRUMENT_NAME,
  IS_ACTIVE,
  KIND,
  MAKER_COMMISSION,
  MAX_LEVERAGE,
  MIN_TRADE_AMOUNT,
  OPTION_TYPE,
  QUOTE_CURRENCY,
  SETTLEMENT_PERIOD,
  STRIKE,
  TAKER_COMMISSION,
  TICK_SIZE,
};

constexpr Field parse_b(auto& name) {
  if (name.compare("base_currency") == 0)
    return Field::BASE_CURRENCY;
  return Field::UNKNOWN;
}

constexpr Field parse_c(auto& name) {
  if (name.length() >= 2) {
    switch (name.data()[1]) {
      case 'o':
        if (name.compare("contract_size") == 0)
          return Field::CONTRACT_SIZE;
        break;
      case 'r':
        if (name.compare("creation_timestamp") == 0)
          return Field::CREATION_TIMESTAMP;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_e(auto& name) {
  if (name.compare("expiration_timestamp") == 0)
    return Field::EXPIRATION_TIMESTAMP;
  return Field::UNKNOWN;
}

constexpr Field parse_i(auto& name) {
  if (name.length() >= 2) {
    switch (name.data()[1]) {
      case 'n':
        if (name.compare("instrument_name") == 0)
          return Field::INSTRUMENT_NAME;
        break;
      case 's':
        if (name.compare("is_active") == 0)
          return Field::IS_ACTIVE;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_k(auto& name) {
  if (name.compare("kind") == 0)
    return Field::KIND;
  return Field::UNKNOWN;
}

constexpr Field parse_m(auto& name) {
  if (name.length() >= 3) {
    switch (name.data()[2]) {
      case 'k':
        if (name.compare("maker_commission") == 0)
          return Field::MAKER_COMMISSION;
        break;
      case 'x':
        if (name.compare("max_leverage") == 0)
          return Field::MAX_LEVERAGE;
        break;
      case 'n':
        if (name.compare("min_trade_amount") == 0)
          return Field::MIN_TRADE_AMOUNT;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_o(auto& name) {
  if (name.compare("option_type") == 0)
    return Field::OPTION_TYPE;
  return Field::UNKNOWN;
}

constexpr Field parse_q(auto& name) {
  if (name.compare("quote_currency") == 0)
    return Field::QUOTE_CURRENCY;
  return Field::UNKNOWN;
}

constexpr Field parse_s(auto& name) {
  if (name.length() >= 2) {
    switch (name.data()[1]) {
      case 'e':
        if (name.compare("settlement_period") == 0)
          return Field::SETTLEMENT_PERIOD;
        break;
      case 't':
        if (name.compare("strike") == 0)
          return Field::STRIKE;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_t(auto& name) {
  if (name.length() >= 2) {
    switch (name.data()[1]) {
      case 'a':
        if (name.compare("taker_commission") == 0)
          return Field::TAKER_COMMISSION;
        break;
      case 'i':
        if (name.compare("tick_size") == 0)
          return Field::TICK_SIZE;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_name(const std::string_view& name) {
  assert(name.empty() == false);
  switch (name.data()[0]) {
    case 'b':
      return parse_b(name);
    case 'c':
      return parse_c(name);
    case 'e':
      return parse_e(name);
    case 'i':
      return parse_i(name);
    case 'k':
      return parse_k(name);
    case 'm':
      return parse_m(name);
    case 'o':
      return parse_o(name);
    case 'q':
      return parse_q(name);
    case 's':
      return parse_s(name);
    case 't':
      return parse_t(name);
  }
  return Field::UNKNOWN;
}

static_assert(parse_name("base_currency") == Field::BASE_CURRENCY);
static_assert(parse_name("contract_size") == Field::CONTRACT_SIZE);
static_assert(parse_name("creation_timestamp") == Field::CREATION_TIMESTAMP);
static_assert(parse_name("expiration_timestamp") == Field::EXPIRATION_TIMESTAMP);
static_assert(parse_name("instrument_name") == Field::INSTRUMENT_NAME);
static_assert(parse_name("is_active") == Field::IS_ACTIVE);
static_assert(parse_name("kind") == Field::KIND);
static_assert(parse_name("maker_commission") == Field::MAKER_COMMISSION);
static_assert(parse_name("max_leverage") == Field::MAX_LEVERAGE);
static_assert(parse_name("min_trade_amount") == Field::MIN_TRADE_AMOUNT);
static_assert(parse_name("option_type") == Field::OPTION_TYPE);
static_assert(parse_name("quote_currency") == Field::QUOTE_CURRENCY);
static_assert(parse_name("settlement_period") == Field::SETTLEMENT_PERIOD);
static_assert(parse_name("strike") == Field::STRIKE);
static_assert(parse_name("taker_commission") == Field::TAKER_COMMISSION);
static_assert(parse_name("tick_size") == Field::TICK_SIZE);

inline void update_field(auto& result, auto& field, auto& value) {
  switch (field) {
    case Field::UNKNOWN: {
      break;
    }
    case Field::BASE_CURRENCY: {
      update(result.base_currency, value);
      break;
    }
    case Field::CONTRACT_SIZE: {
      update(result.contract_size, value);
      break;
    }
    case Field::CREATION_TIMESTAMP: {
      update(result.creation_timestamp, value);
      break;
    }
    case Field::EXPIRATION_TIMESTAMP: {
      update(result.expiration_timestamp, value);
      break;
    }
    case Field::INSTRUMENT_NAME: {
      update(result.instrument_name, value);
      break;
    }
    case Field::IS_ACTIVE: {
      update(result.is_active, value);
      break;
    }
    case Field::KIND: {
      update(result.kind, value);
      break;
    }
    case Field::MAKER_COMMISSION: {
      update(result.maker_commission, value);
      break;
    }
    case Field::MAX_LEVERAGE: {
      update(result.max_leverage, value);
      break;
    }
    case Field::MIN_TRADE_AMOUNT: {
      update(result.min_trade_amount, value);
      break;
    }
    case Field::OPTION_TYPE: {
      update(result.option_type, value);
      break;
    }
    case Field::QUOTE_CURRENCY: {
      update(result.quote_currency, value);
      break;
    }
    case Field::SETTLEMENT_PERIOD: {
      update(result.settlement_period, value);
      break;
    }
    case Field::STRIKE: {
      update(result.strike, value);
      break;
    }
    case Field::TAKER_COMMISSION: {
      update(result.taker_commission, value);
      break;
    }
    case Field::TICK_SIZE: {
      update(result.tick_size, value);
      break;
    }
  }
}
}  // namespace

void Instrument::parse(Instrument& result, core::json::object_t&& object) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  for (auto [key, value] : object) {
    auto field = parse_name(key);
    update_field(result, field, value);
  }
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
