/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/json/ticker.h"

#include "roq/core/json/parser.h"

#include "roq/deribit/json/utils.h"

namespace roq {
namespace deribit {
namespace json {

namespace {
enum class Field {
  UNKNOWN,
  BEST_ASK_AMOUNT,
  BEST_ASK_PRICE,
  BEST_BID_AMOUNT,
  BEST_BID_PRICE,
  CURRENT_FUNDING,
  FUNDING_8H,
  INSTRUMENT_NAME,
  LAST_PRICE,
  MARK_PRICE,
  MAX_PRICE,
  MIN_PRICE,
  OPEN_INTEREST,
  SETTLEMENT_PRICE,
  STATE,
  STATS,
  TIMESTAMP,
};

constexpr Field parse_b(auto& name) {
  if (name.length() >= 10) {
    switch (name.data()[5]) {
      case 'a':
        switch (name.data()[9]) {
          case 'a':
            if (name.compare("best_ask_amount") == 0)
              return Field::BEST_ASK_AMOUNT;
            break;
          case 'p':
            if (name.compare("best_ask_price") == 0)
              return Field::BEST_ASK_PRICE;
            break;
        }
        break;
      case 'b':
        switch (name.data()[9]) {
          case 'a':
            if (name.compare("best_bid_amount") == 0)
              return Field::BEST_BID_AMOUNT;
            break;
          case 'p':
            if (name.compare("best_bid_price") == 0)
              return Field::BEST_BID_PRICE;
            break;
        }
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_c(auto& name) {
  if (name.compare("current_funding") == 0)
    return Field::CURRENT_FUNDING;
  return Field::UNKNOWN;
}

constexpr Field parse_f(auto& name) {
  if (name.compare("funding_8h") == 0)
    return Field::FUNDING_8H;
  return Field::UNKNOWN;
}

constexpr Field parse_i(auto& name) {
  if (name.compare("instrument_name") == 0)
    return Field::INSTRUMENT_NAME;
  return Field::UNKNOWN;
}

constexpr Field parse_l(auto& name) {
  if (name.compare("last_price") == 0)
    return Field::LAST_PRICE;
  return Field::UNKNOWN;
}

constexpr Field parse_m(auto& name) {
  if (name.length() >= 3) {
    switch (name.data()[2]) {
      case 'r':
        if (name.compare("mark_price") == 0)
          return Field::MARK_PRICE;
        break;
      case 'x':
        if (name.compare("max_price") == 0)
          return Field::MAX_PRICE;
        break;
      case 'n':
        if (name.compare("min_price") == 0)
          return Field::MIN_PRICE;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_o(auto& name) {
  if (name.compare("open_interest") == 0)
    return Field::OPEN_INTEREST;
  return Field::UNKNOWN;
}

constexpr Field parse_s(auto& name) {
  if (name.length() >= 5) {
    switch (name.data()[4]) {
      case 'l':
        if (name.compare("settlement_price") == 0)
          return Field::SETTLEMENT_PRICE;
        break;
      case 'e':
        if (name.compare("state") == 0)
          return Field::STATE;
        break;
      case 's':
        if (name.compare("stats") == 0)
          return Field::STATS;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_t(auto& name) {
  if (name.compare("timestamp") == 0)
    return Field::TIMESTAMP;
  return Field::UNKNOWN;
}

constexpr Field parse_name(const std::string_view& name) {
  assert(name.empty() == false);
  switch (name.data()[0]) {
    case 'b':
      return parse_b(name);
    case 'c':
      return parse_c(name);
    case 'f':
      return parse_f(name);
    case 'i':
      return parse_i(name);
    case 'l':
      return parse_l(name);
    case 'm':
      return parse_m(name);
    case 'o':
      return parse_o(name);
    case 's':
      return parse_s(name);
    case 't':
      return parse_t(name);
  }
  return Field::UNKNOWN;
}

static_assert(parse_name("best_ask_amount") == Field::BEST_ASK_AMOUNT);
static_assert(parse_name("best_ask_price") == Field::BEST_ASK_PRICE);
static_assert(parse_name("best_bid_amount") == Field::BEST_BID_AMOUNT);
static_assert(parse_name("best_bid_price") == Field::BEST_BID_PRICE);
static_assert(parse_name("current_funding") == Field::CURRENT_FUNDING);
static_assert(parse_name("funding_8h") == Field::FUNDING_8H);
static_assert(parse_name("instrument_name") == Field::INSTRUMENT_NAME);
static_assert(parse_name("last_price") == Field::LAST_PRICE);
static_assert(parse_name("mark_price") == Field::MARK_PRICE);
static_assert(parse_name("max_price") == Field::MAX_PRICE);
static_assert(parse_name("min_price") == Field::MIN_PRICE);
static_assert(parse_name("open_interest") == Field::OPEN_INTEREST);
static_assert(parse_name("settlement_price") == Field::SETTLEMENT_PRICE);
static_assert(parse_name("state") == Field::STATE);
static_assert(parse_name("stats") == Field::STATS);
static_assert(parse_name("timestamp") == Field::TIMESTAMP);

inline void update_field(auto& result, auto& field, auto& value) {
  switch (field) {
    case Field::UNKNOWN: {
      break;
    }
    case Field::BEST_ASK_AMOUNT: {
      update(result.best_ask_amount, value);
      break;
    }
    case Field::BEST_ASK_PRICE: {
      update(result.best_ask_price, value);
      break;
    }
    case Field::BEST_BID_AMOUNT: {
      update(result.best_bid_amount, value);
      break;
    }
    case Field::BEST_BID_PRICE: {
      update(result.best_bid_price, value);
      break;
    }
    case Field::CURRENT_FUNDING: {
      update(result.current_funding, value);
      break;
    }
    case Field::FUNDING_8H: {
      update(result.funding_8h, value);
      break;
    }
    case Field::INSTRUMENT_NAME: {
      update(result.instrument_name, value);
      break;
    }
    case Field::LAST_PRICE: {
      update(result.last_price, value);
      break;
    }
    case Field::MARK_PRICE: {
      update(result.mark_price, value);
      break;
    }
    case Field::MAX_PRICE: {
      update(result.max_price, value);
      break;
    }
    case Field::MIN_PRICE: {
      update(result.min_price, value);
      break;
    }
    case Field::OPEN_INTEREST: {
      update(result.open_interest, value);
      break;
    }
    case Field::SETTLEMENT_PRICE: {
      update(result.settlement_price, value);
      break;
    }
    case Field::STATE: {
      update(result.state, value);
      break;
    }
    case Field::STATS: {
      // ????????????????????????????????
      break;
    }
    case Field::TIMESTAMP: {
      update(result.timestamp, value);
      break;
    }
  }
}
}  // namespace

Ticker Ticker::parse_message(const std::string_view& message) {
  Ticker result;
  core::json::Parser parser(message);
  for (auto [key, value] : parser.root<core::json::object_t>()) {
    auto field = parse_name(key);
    update_field(result, field, value);
  }
  return result;
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
