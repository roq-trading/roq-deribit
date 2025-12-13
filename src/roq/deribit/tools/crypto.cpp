/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/tools/crypto.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <random>
#include <span>

#include <fmt/format.h>

#include "roq/logging.hpp"

#include "roq/utils/codec/base64.hpp"
#include "roq/utils/codec/hex.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace tools {

// === CONSTANTS ===

namespace {
constexpr size_t const RANDOM_BYTES = 32;

constexpr auto const CHARSET_DATA = "abcdefghijklmnopqrstuvwxyz0123456789"sv;
constexpr size_t const LENGTH_CHARSET_DATA = 36;
static_assert(std::size(CHARSET_DATA) == LENGTH_CHARSET_DATA);

std::random_device GENERATOR;
std::uniform_int_distribution<int> CHARSET_DISTRIBUTION(0, std::size(CHARSET_DATA) - 1);  // note! max value is inclusive
std::uniform_int_distribution<uint32_t> DISTRIBUTION;
}  // namespace

// === IMPLEMENTATION ===

Crypto::Crypto(std::string_view const &access_secret) : secret_{access_secret}, mac_{secret_} {
  if (std::empty(secret_)) {
    log::fatal("The API secret key is required"sv);
  }
}

std::string Crypto::create_nonce() {
  std::string result;
  result.resize(RANDOM_BYTES);
  assert(std::size(result) == RANDOM_BYTES);
  std::ranges::generate(result, []() { return CHARSET_DATA[CHARSET_DISTRIBUTION(GENERATOR)]; });
  return result;
}

std::pair<std::string, std::chrono::milliseconds> Crypto::create_signature(std::chrono::milliseconds timestamp, std::string_view const &nonce) {
  auto sequence = get_sequence(timestamp);
  auto message = fmt::format("{}\n{}\n"sv, sequence, nonce);
  mac_.clear();
  mac_.update(message);
  auto digest = mac_.final(digest_);
  std::string result;
  utils::codec::Hex::encode(result, digest);
  return {result, std::chrono::milliseconds{sequence}};
}

std::string Crypto::create_raw_data(std::chrono::milliseconds timestamp) {
  using value_type = decltype(DISTRIBUTION)::result_type;
  constexpr auto length = RANDOM_BYTES / sizeof(value_type);
  std::array<value_type, length> buffer;
  for (size_t i = 0; i < length; ++i) {
    buffer[i] = DISTRIBUTION(GENERATOR);
  }
  std::span tmp{reinterpret_cast<std::byte *>(std::data(buffer)), std::size(buffer) * sizeof(value_type)};
  std::string nonce;
  utils::codec::Base64::encode(nonce, tmp, false, false);
  return create_raw_data(timestamp, nonce);
}

std::string Crypto::create_raw_data(std::chrono::milliseconds timestamp, std::string_view const &nonce) {
  auto sequence = get_sequence(timestamp);
  auto raw_data = fmt::format("{:013}.{}"sv, sequence, nonce);
  return raw_data;
}

std::string Crypto::create_password(std::string_view const &raw_data) {
  hash_.clear();
  hash_.update(raw_data);
  hash_.update(secret_);
  std::array<std::byte, Hash::DIGEST_LENGTH> buffer;
  auto digest = hash_.final(buffer);
  std::string result;
  utils::codec::Base64::encode(result, digest, false, false);
  return result;
}

int64_t Crypto::get_sequence(std::chrono::milliseconds timestamp) {
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
