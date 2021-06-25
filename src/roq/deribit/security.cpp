/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/security.h"

#include <array>
#include <random>

#include "roq/format.h"
#include "roq/literals.h"

#include "roq/core/binascii/base64.h"
#include "roq/core/binascii/hex.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

namespace {
static const constexpr auto CHARSET_DATA = "abcdefghijklmnopqrstuvwxyz0123456789"_sv;
static const constexpr auto RANDOM_BYTES = 32;

static_assert(std::size(CHARSET_DATA) == 36);

static std::random_device GENERATOR;
static std::uniform_int_distribution<int> CHARSET_DISTRIBUTION(
    0, std::size(CHARSET_DATA) - 1);  // note! max value is inclusive
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
}  // namespace

Security::Security(
    const std::string_view &account,
    const std::string_view &access_key,
    const std::string_view &access_secret)
    : account_(account), key_(access_key), secret_(access_secret), hmac_(secret_) {
}

std::string Security::create_nonce() {
  std::string result(RANDOM_BYTES, '-');
  std::generate(
      result.begin(), result.end(), []() { return CHARSET_DATA[CHARSET_DISTRIBUTION(GENERATOR)]; });
  return result;
}

std::string Security::create_signature(
    std::chrono::milliseconds timestamp, const std::string_view &nonce) {
  auto sequence = get_sequence(timestamp);
  auto message = roq::format("{}\n{}\n"_sv, sequence, nonce);
  hmac_.clear();
  hmac_.update(message);
  std::array<char, 32> buffer;
  auto length = hmac_.digest(buffer);
  assert(length == buffer.size());
  return core::binascii::Hex::encode(buffer);
}

std::string Security::create_raw_data(std::chrono::milliseconds timestamp) {
  auto sequence = get_sequence(timestamp);
  // create nonce
  using value_type = decltype(DISTRIBUTION)::result_type;
  constexpr auto n = RANDOM_BYTES / sizeof(value_type);
  std::array<value_type, n> buffer;
  for (size_t i = {}; i < n; ++i)
    buffer[i] = DISTRIBUTION(GENERATOR);
  auto nonce = core::binascii::Base64::encode(buffer.data(), buffer.size() * sizeof(value_type));
  return roq::format("{:013}.{}"_sv, sequence, nonce);
}

std::string Security::create_password(const std::string_view &raw_data) {
  sha_.clear();
  sha_.update(raw_data);
  sha_.update(secret_);
  std::array<char, 32> buffer;
  auto length = sha_.digest(buffer);
  assert(length == buffer.size());
  return core::binascii::Base64::encode(buffer);
}

int64_t Security::get_sequence(std::chrono::milliseconds timestamp) {
  if (timestamp_ < timestamp) {
    timestamp_ = timestamp;
  } else {
    ++timestamp_;
  }
  return timestamp_.count();
}

}  // namespace deribit
}  // namespace roq
