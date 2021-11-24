/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/tools/hasher.h"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <random>

#include "roq/span.h"

#include "roq/core/binascii/base64.h"
#include "roq/core/binascii/hex.h"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace tools {

namespace {
static const constexpr auto CHARSET_DATA = "abcdefghijklmnopqrstuvwxyz0123456789"sv;
static const constexpr auto RANDOM_BYTES = 32;

static_assert(std::size(CHARSET_DATA) == 36);

static std::random_device GENERATOR;
static std::uniform_int_distribution<int> CHARSET_DISTRIBUTION(
    0, std::size(CHARSET_DATA) - 1);  // note! max value is inclusive
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
}  // namespace

Hasher::Hasher(const std::string_view &access_secret) : secret_(access_secret), hmac_(secret_) {
}

std::string Hasher::create_nonce() {
  std::string result(RANDOM_BYTES, '-');
  std::generate(
      result.begin(), result.end(), []() { return CHARSET_DATA[CHARSET_DISTRIBUTION(GENERATOR)]; });
  return result;
}

std::pair<std::string, std::chrono::milliseconds> Hasher::create_signature(
    std::chrono::milliseconds timestamp, const std::string_view &nonce) {
  auto sequence = get_sequence(timestamp);
  auto message = fmt::format("{}\n{}\n"sv, sequence, nonce);
  hmac_.clear();
  hmac_.update(message);
  std::array<char, 32> buffer;
  auto length = hmac_.digest(buffer);
  assert(length == buffer.size());
  return {core::binascii::Hex::encode(buffer), std::chrono::milliseconds{sequence}};
}

std::pair<std::string, std::chrono::milliseconds> Hasher::create_raw_data(
    std::chrono::milliseconds timestamp) {
  auto sequence = get_sequence(timestamp);
  // create nonce
  using value_type = decltype(DISTRIBUTION)::result_type;
  constexpr auto n = RANDOM_BYTES / sizeof(value_type);
  std::array<value_type, n> buffer;
  for (size_t i = {}; i < n; ++i)
    buffer[i] = DISTRIBUTION(GENERATOR);
  roq::span tmp{
      reinterpret_cast<std::byte *>(std::data(buffer)), std::size(buffer) * sizeof(value_type)};
  auto nonce = core::binascii::Base64::encode(tmp, false);
  auto raw_data = fmt::format("{:013}.{}"sv, sequence, nonce);
  return {raw_data, std::chrono::milliseconds{sequence}};
}

std::string Hasher::create_password(const std::string_view &raw_data) {
  sha_.clear();
  sha_.update(raw_data);
  sha_.update(secret_);
  std::array<char, 32> buffer;
  auto length = sha_.digest(buffer);
  assert(length == buffer.size());
  return core::binascii::Base64::encode(buffer, false);
}

int64_t Hasher::get_sequence(std::chrono::milliseconds timestamp) {
  if (timestamp_ < timestamp) {
    timestamp_ = timestamp;
  } else {
    ++timestamp_;
  }
  return timestamp_.count();
}

}  // namespace tools
}  // namespace deribit
}  // namespace roq
