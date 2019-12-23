/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstddef>

namespace roq {
namespace deribit {
namespace api {

template <typename T>
T to_enum(const std::string_view&);

// --> Kind

enum class Kind {
  UNKNOWN,
  FUTURE,
  OPTION,
};

inline const char * const *EnumNamesKind() {
  static const char * const names[] = {
    "UNKNOWN",
    "FUTURE",
    "OPTION",
  };
  return names;
}

inline const char *EnumNameKind(const Kind& e) {
  const size_t index = static_cast<int>(e);
  return EnumNamesKind()[index];
}

template <>
api::Kind to_enum(const std::string_view& value);

// --> OptionType

enum class OptionType {
  UNKNOWN,
  CALL,
  PUT,
};

inline const char * const *EnumNamesOptionType() {
  static const char * const names[] = {
    "UNKNOWN",
    "CALL",
    "PUT",
  };
  return names;
}

inline const char *EnumNameOptionType(const OptionType& e) {
  const size_t index = static_cast<int>(e);
  return EnumNamesOptionType()[index];
}

template <>
api::OptionType to_enum(const std::string_view& value);

// --> State

enum class State {
  UNKNOWN,
  OPEN,
  CLOSED,
};

inline const char * const *EnumNamesState() {
  static const char * const names[] = {
    "UNKNOWN",
    "OPEN",
    "CLOSED",
  };
  return names;
}

inline const char *EnumNameState(const State& e) {
  const size_t index = static_cast<int>(e);
  return EnumNamesState()[index];
}

template <>
api::State to_enum(const std::string_view& value);

}  // namespace api
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::api::Kind> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::api::Kind& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{}",
        roq::deribit::api::EnumNameKind(value));
  }
};

template <>
struct fmt::formatter<roq::deribit::api::OptionType> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::api::OptionType& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{}",
        roq::deribit::api::EnumNameOptionType(value));
  }
};

template <>
struct fmt::formatter<roq::deribit::api::State> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::api::State& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{}",
        roq::deribit::api::EnumNameState(value));
  }
};
