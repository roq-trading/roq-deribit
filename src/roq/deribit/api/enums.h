/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstddef>

namespace roq {
namespace deribit {
namespace api {

template <typename T>
T to_enum(const std::string_view&);

// --> State

enum class State {
  UNKNOWN,
  OPEN,
};

inline const char * const *EnumNamesState() {
  static const char * const names[] = {
    "UNKNOWN",
    "OPEN",
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
struct fmt::formatter<roq::deribit::api::State> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::api::State& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{}",
        roq::deribit::api::EnumNameState(value));
  }
};
