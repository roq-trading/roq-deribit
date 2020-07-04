/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/random.h"

#include <fmt/format.h>

#include <array>
#include <random>

#include "roq/core/binascii/base64.h"
#include "roq/core/binascii/hex.h"

namespace roq {
namespace deribit {

static char CHARSET_DATA[] =
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789";

constexpr auto CHARSET_LENGTH = sizeof(CHARSET_DATA) - 1;

static_assert(CHARSET_LENGTH == 36);

static std::random_device GENERATOR;

static std::uniform_int_distribution<int> CHARSET_DISTRIBUTION(
    0,
    CHARSET_LENGTH - 1);  // note! inclusive

static std::uniform_int_distribution<uint32_t> DISTRIBUTION;

constexpr size_t RANDOM_BYTES = 32;

Random::Random(const std::string_view& secret)
    : _secret(secret),
      _hmac(
          secret.data(),
          secret.length()) {
}

std::string Random::create_nonce() {
  std::string result(RANDOM_BYTES, '-');
  for (auto& iter : result)
    iter = CHARSET_DATA[CHARSET_DISTRIBUTION(GENERATOR)];
  return result;
}

std::string Random::create_signature(
    std::chrono::milliseconds timestamp,
    const std::string_view& nonce) {
  auto message = fmt::format(
      "{}\n{}\n",
      timestamp.count(),
      nonce);
  _hmac.clear();
  _hmac.update(message);
  std::array<char, 32> buffer;
  auto length = _hmac.digest(
      buffer.data(),
      buffer.size());
  assert(length == buffer.size());
  return core::binascii::Hex::encode(
      buffer.data(),
      length);
}

std::string Random::create_raw_data(
    const std::chrono::nanoseconds now) {
  using value_type = decltype(DISTRIBUTION)::result_type;
  constexpr auto n = RANDOM_BYTES / sizeof(value_type);
  std::array<value_type, n> buffer;
  for (size_t i = 0; i < n; ++i)
    buffer[i] = DISTRIBUTION(GENERATOR);
  auto nonce = core::binascii::Base64::encode(
      buffer.data(),
      buffer.size() * sizeof(value_type));
  auto msecs = std::chrono::duration_cast<
    std::chrono::milliseconds>(now).count();
  return fmt::format(
      "{:013}.{}",
      msecs,
      nonce);
}

std::string Random::create_password(const std::string_view& raw_data) {
  _sha.clear();
  _sha.update(raw_data);
  _sha.update(_secret);
  std::array<char, 32> buffer;
  auto length = _sha.digest(
      buffer.data(),
      buffer.size());
  assert(length == buffer.size());
  return core::binascii::Base64::encode(
      buffer.data(),
      length);
}

}  // namespace deribit
}  // namespace roq
