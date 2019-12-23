/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/json/currency.h"

#include "roq/core/json/parser.h"

#include "roq/deribit/json/utils.h"

namespace roq {
namespace deribit {
namespace json {

namespace {
enum class Field {
  UNKNOWN,
  COIN_TYPE,
  CURRENCY,
  CURRENCY_LONG,
  DISABLED_DEPOSIT_ADDRESS_CREATION,
  FEE_PRECISION,
  MIN_CONFIRMATIONS,
  MIN_WITHDRAWAL_FEE,
  WITHDRAWAL_FEE,
  WITHDRAWAL_PRIORITIES,
};

constexpr Field parse_c(auto& name) {
  if (name.length() >= 2) {
    switch (name.data()[1]) {
      case 'o':
        if (name.compare("coin_type") == 0)
          return Field::COIN_TYPE;
        break;
      case 'u':
        if (name.length() > 8 && name.compare("currency_long") == 0)
          return Field::CURRENCY_LONG;
        else if (name.compare("currency") == 0)
          return Field::CURRENCY;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_d(auto& name) {
  if (name.compare("disabled_deposit_address_creation") == 0)
    return Field::DISABLED_DEPOSIT_ADDRESS_CREATION;
  return Field::UNKNOWN;
}

constexpr Field parse_f(auto& name) {
  if (name.compare("fee_precision") == 0)
    return Field::FEE_PRECISION;
  return Field::UNKNOWN;
}

constexpr Field parse_m(auto& name) {
  if (name.length() >= 5) {
    switch (name.data()[4]) {
      case 'c':
        if (name.compare("min_confirmations") == 0)
          return Field::MIN_CONFIRMATIONS;
        break;
      case 'w':
        if (name.compare("min_withdrawal_fee") == 0)
          return Field::MIN_WITHDRAWAL_FEE;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_w(auto& name) {
  if (name.length() >= 12) {
    switch (name.data()[11]) {
      case 'f':
        if (name.compare("withdrawal_fee") == 0)
          return Field::WITHDRAWAL_FEE;
        break;
      case 'p':
        if (name.compare("withdrawal_priorities") == 0)
          return Field::WITHDRAWAL_PRIORITIES;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_name(const std::string_view& name) {
  assert(name.empty() == false);
  switch (name.data()[0]) {
    case 'c':
      return parse_c(name);
    case 'd':
      return parse_d(name);
    case 'f':
      return parse_f(name);
    case 'm':
      return parse_m(name);
    case 'w':
      return parse_w(name);
  }
  return Field::UNKNOWN;
}

static_assert(parse_name("coin_type") == Field::COIN_TYPE);
static_assert(parse_name("currency") == Field::CURRENCY);
static_assert(parse_name("currency_long") == Field::CURRENCY_LONG);
static_assert(parse_name("disabled_deposit_address_creation") == Field::DISABLED_DEPOSIT_ADDRESS_CREATION);
static_assert(parse_name("fee_precision") == Field::FEE_PRECISION);
static_assert(parse_name("min_confirmations") == Field::MIN_CONFIRMATIONS);
static_assert(parse_name("min_withdrawal_fee") == Field::MIN_WITHDRAWAL_FEE);
static_assert(parse_name("withdrawal_fee") == Field::WITHDRAWAL_FEE);
static_assert(parse_name("withdrawal_priorities") == Field::WITHDRAWAL_PRIORITIES);

inline void update_field(auto& result, auto& field, auto& value) {
  switch (field) {
    case Field::UNKNOWN: {
      break;
    }
    case Field::COIN_TYPE: {
      update(result.coin_type, value);
      break;
    }
    case Field::CURRENCY: {
      update(result.currency, value);
      break;
    }
    case Field::CURRENCY_LONG: {
      update(result.currency_long, value);
      break;
    }
    case Field::DISABLED_DEPOSIT_ADDRESS_CREATION: {
      update(result.disabled_deposit_address_creation, value);
      break;
    }
    case Field::FEE_PRECISION: {
      update(result.fee_precision, value);
      break;
    }
    case Field::MIN_CONFIRMATIONS: {
      update(result.min_confirmations, value);
      break;
    }
    case Field::MIN_WITHDRAWAL_FEE: {
      update(result.min_withdrawal_fee, value);
      break;
    }
    case Field::WITHDRAWAL_FEE: {
      update(result.withdrawal_fee, value);
      break;
    }
    case Field::WITHDRAWAL_PRIORITIES: {
      // TODO(thraneh): implement
      break;
    }
  }
}
}  // namespace

void Currency::parse(Currency& result, core::json::object_t&& object) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  for (auto [key, value] : object) {
    auto field = parse_name(key);
    update_field(result, field, value);
  }
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
