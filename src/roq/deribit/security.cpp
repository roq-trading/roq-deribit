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
static const constexpr auto RANDOM_BYTES = 32u;

static_assert(std::size(CHARSET_DATA) == 36u);

static std::random_device GENERATOR;
static std::uniform_int_distribution<int> CHARSET_DISTRIBUTION(
    0, std::size(CHARSET_DATA) - 1);  // note! max value is inclusive
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
}  // namespace

Security::Security(const Config &config)
    : key_(config.get_access_key()), secret_(config.get_access_secret()), hmac_(secret_) {
}

std::string Security::create_nonce() {
  std::string result(RANDOM_BYTES, '-');
  std::generate(
      result.begin(), result.end(), []() { return CHARSET_DATA[CHARSET_DISTRIBUTION(GENERATOR)]; });
  return result;
}

std::string Security::create_signature(
    std::chrono::milliseconds timestamp, const std::string_view &nonce) {
  auto message = roq::format("{}\n{}\n"_fmt, timestamp.count(), nonce);
  hmac_.clear();
  hmac_.update(message);
  std::array<char, 32u> buffer;
  auto length = hmac_.digest(buffer);
  assert(length == buffer.size());
  return core::binascii::Hex::encode(buffer);
}

std::string Security::create_raw_data(const std::chrono::nanoseconds now) {
  using value_type = decltype(DISTRIBUTION)::result_type;
  constexpr auto n = RANDOM_BYTES / sizeof(value_type);
  std::array<value_type, n> buffer;
  for (size_t i = {}; i < n; ++i)
    buffer[i] = DISTRIBUTION(GENERATOR);
  auto nonce = core::binascii::Base64::encode(buffer.data(), buffer.size() * sizeof(value_type));
  auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  return roq::format("{:013}.{}"_fmt, msecs, nonce);
}

std::string Security::create_password(const std::string_view &raw_data) {
  sha_.clear();
  sha_.update(raw_data);
  sha_.update(secret_);
  std::array<char, 32u> buffer;
  auto length = sha_.digest(buffer);
  assert(length == buffer.size());
  return core::binascii::Base64::encode(buffer);
}

}  // namespace deribit
}  // namespace roq
