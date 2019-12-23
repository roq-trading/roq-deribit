/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/json/order_book.h"

#include "roq/core/json/parser.h"

#include "roq/deribit/json/utils.h"

namespace roq {
namespace deribit {
namespace json {

namespace {
enum class Field {
  UNKNOWN,
  ASK_IV,
  ASKS,
  BEST_ASK_AMOUNT,
  BEST_ASK_PRICE,
  BEST_BID_AMOUNT,
  BEST_BID_PRICE,
  BID_IV,
  BIDS,
  CURRENT_FUNDING,
  DELIVERY_PRICE,
  FUNDING_8H,
  GREEKS,
  INDEX_PRICE,
  INSTRUMENT_NAME,
  INTEREST_RATE,
  LAST_PRICE,
  MARK_IV,
  MARK_PRICE,
  MAX_PRICE,
  MIN_PRICE,
  OPEN_INTEREST,
  SETTLEMENT_PRICE,
  STATE,
  STATS,
  TIMESTAMP,
  UNDERLYING_INDEX,
  UNDERLYING_PRICE,
};

constexpr Field parse_a(auto& name) {
  if (name.length() >= 4) {
    switch (name.data()[3]) {
      case '_':
        if (name.compare("ask_iv") == 0)
          return Field::ASK_IV;
        break;
      case 's':
        if (name.compare("asks") == 0)
          return Field::ASKS;
        break;
    }
  }
  return Field::UNKNOWN;
}

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
  } else if (name.length() >=4) {
    switch (name.data()[3]) {
      case '_':
        if (name.compare("bid_iv") == 0)
          return Field::BID_IV;
        break;
      case 's':
        if (name.compare("bids") == 0)
          return Field::BIDS;
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

constexpr Field parse_d(auto& name) {
  if (name.compare("delivery_price") == 0)
    return Field::DELIVERY_PRICE;
  return Field::UNKNOWN;
}

constexpr Field parse_f(auto& name) {
  if (name.compare("funding_8h") == 0)
    return Field::FUNDING_8H;
  return Field::UNKNOWN;
}

constexpr Field parse_g(auto& name) {
  if (name.compare("greeks") == 0)
    return Field::GREEKS;
  return Field::UNKNOWN;
}

constexpr Field parse_i(auto& name) {
  if (name.length() >=3) {
    switch (name.data()[2]) {
      case 'd':
        if (name.compare("index_price") == 0)
          return Field::INDEX_PRICE;
        break;
      case 's':
        if (name.compare("instrument_name") == 0)
          return Field::INSTRUMENT_NAME;
        break;
      case 't':
        if (name.compare("interest_rate") == 0)
          return Field::INTEREST_RATE;
        break;
    }
  }
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
        if (name.length() >= 6) {
          switch (name.data()[5]) {
            case 'i':
              if (name.compare("mark_iv") == 0)
                return Field::MARK_IV;
              break;
            case 'p':
              if (name.compare("mark_price") == 0)
                return Field::MARK_PRICE;
              break;
          }
        }
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

constexpr Field parse_u(auto& name) {
  if (name.length() >= 12) {
    switch (name.data()[11]) {
      case 'i':
        if (name.compare("underlying_index") == 0)
          return Field::UNDERLYING_INDEX;
        break;
      case 'p':
        if (name.compare("underlying_price") == 0)
          return Field::UNDERLYING_PRICE;
        break;
    }
  }
  return Field::UNKNOWN;
}

constexpr Field parse_name(const std::string_view& name) {
  assert(name.empty() == false);
  switch (name.data()[0]) {
    case 'a':
      return parse_a(name);
    case 'b':
      return parse_b(name);
    case 'c':
      return parse_c(name);
    case 'd':
      return parse_d(name);
    case 'f':
      return parse_f(name);
    case 'g':
      return parse_g(name);
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
    case 'u':
      return parse_u(name);
  }
  return Field::UNKNOWN;
}

static_assert(parse_name("ask_iv") == Field::ASK_IV);
static_assert(parse_name("asks") == Field::ASKS);
static_assert(parse_name("best_ask_amount") == Field::BEST_ASK_AMOUNT);
static_assert(parse_name("best_ask_price") == Field::BEST_ASK_PRICE);
static_assert(parse_name("best_bid_amount") == Field::BEST_BID_AMOUNT);
static_assert(parse_name("best_bid_price") == Field::BEST_BID_PRICE);
static_assert(parse_name("bid_iv") == Field::BID_IV);
static_assert(parse_name("bids") == Field::BIDS);
static_assert(parse_name("current_funding") == Field::CURRENT_FUNDING);
static_assert(parse_name("delivery_price") == Field::DELIVERY_PRICE);
static_assert(parse_name("funding_8h") == Field::FUNDING_8H);
static_assert(parse_name("greeks") == Field::GREEKS);
static_assert(parse_name("index_price") == Field::INDEX_PRICE);
static_assert(parse_name("instrument_name") == Field::INSTRUMENT_NAME);
static_assert(parse_name("interest_rate") == Field::INTEREST_RATE);
static_assert(parse_name("last_price") == Field::LAST_PRICE);
static_assert(parse_name("mark_iv") == Field::MARK_IV);
static_assert(parse_name("mark_price") == Field::MARK_PRICE);
static_assert(parse_name("max_price") == Field::MAX_PRICE);
static_assert(parse_name("min_price") == Field::MIN_PRICE);
static_assert(parse_name("open_interest") == Field::OPEN_INTEREST);
static_assert(parse_name("settlement_price") == Field::SETTLEMENT_PRICE);
static_assert(parse_name("state") == Field::STATE);
static_assert(parse_name("stats") == Field::STATS);
static_assert(parse_name("timestamp") == Field::TIMESTAMP);
static_assert(parse_name("underlying_index") == Field::UNDERLYING_INDEX);
static_assert(parse_name("underlying_price") == Field::UNDERLYING_PRICE);

inline void update_field(auto& result, auto& field, auto& value) {
  switch (field) {
    case Field::UNKNOWN: {
      break;
    }
    case Field::ASK_IV: {
      update(result.ask_iv, value);
      break;
    }
    case Field::ASKS: {
      // update(result.xxx, value);
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
    case Field::BID_IV: {
      update(result.bid_iv, value);
      break;
    }
    case Field::BIDS: {
      // update(result.xxx, value);
      break;
    }
    case Field::CURRENT_FUNDING: {
      update(result.current_funding, value);
      break;
    }
    case Field::DELIVERY_PRICE: {
      update(result.delivery_price, value);
      break;
    }
    case Field::FUNDING_8H: {
      update(result.funding_8h, value);
      break;
    }
    case Field::GREEKS: {
      // update(result.xxx, value);
      break;
    }
    case Field::INDEX_PRICE: {
      update(result.index_price, value);
      break;
    }
    case Field::INSTRUMENT_NAME: {
      update(result.instrument_name, value);
      break;
    }
    case Field::INTEREST_RATE: {
      update(result.interest_rate, value);
      break;
    }
    case Field::LAST_PRICE: {
      update(result.last_price, value);
      break;
    }
    case Field::MARK_IV: {
      update(result.mark_iv, value);
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
      // update(result.xxx, value);
      break;
    }
    case Field::TIMESTAMP: {
      update(result.timestamp, value);
      break;
    }
    case Field::UNDERLYING_INDEX: {
      update(result.underlying_index, value);
      break;
    }
    case Field::UNDERLYING_PRICE: {
      update(result.underlying_price, value);
      break;
    }
  }
}
}  // namespace

void OrderBook::parse(OrderBook& result, core::json::object_t&& object) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  for (auto [key, value] : object) {
    auto field = parse_name(key);
    update_field(result, field, value);
  }
}

}  // namespace json
}  // namespace deribit
}  // namespace roq
