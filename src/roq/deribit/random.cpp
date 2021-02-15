/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/random.h"

#include <array>
#include <random>

#include "roq/format.h"
#include "roq/literals.h"

#include "roq/core/binascii/base64.h"
#include "roq/core/binascii/hex.h"

using namespace roq::literals;

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

Random::Random(const std::string_view &secret)
    : secret_(secret), hmac_(secret.data(), secret.length()) {
}

std::string Random::create_nonce() {
  std::string result(RANDOM_BYTES, '-');
  std::generate(
      result.begin(), result.end(), []() { return CHARSET_DATA[CHARSET_DISTRIBUTION(GENERATOR)]; });
  return result;
}

std::string Random::create_signature(
    std::chrono::milliseconds timestamp, const std::string_view &nonce) {
  auto message = roq::format("{}\n{}\n"_fmt, timestamp.count(), nonce);
  hmac_.clear();
  hmac_.update(message);
  std::array<char, 32> buffer;
  auto length = hmac_.digest(buffer.data(), buffer.size());
  assert(length == buffer.size());
  return core::binascii::Hex::encode(buffer.data(), length);
}

std::string Random::create_raw_data(const std::chrono::nanoseconds now) {
  using value_type = decltype(DISTRIBUTION)::result_type;
  constexpr auto n = RANDOM_BYTES / sizeof(value_type);
  std::array<value_type, n> buffer;
  for (size_t i = 0; i < n; ++i)
    buffer[i] = DISTRIBUTION(GENERATOR);
  auto nonce = core::binascii::Base64::encode(buffer.data(), buffer.size() * sizeof(value_type));
  auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  return roq::format("{:013}.{}"_fmt, msecs, nonce);
}

std::string Random::create_password(const std::string_view &raw_data) {
  sha_.clear();
  sha_.update(raw_data);
  sha_.update(secret_);
  std::array<char, 32> buffer;
  auto length = sha_.digest(buffer.data(), buffer.size());
  assert(length == buffer.size());
  return core::binascii::Base64::encode(buffer.data(), length);
}

}  // namespace deribit
}  // namespace roq
