/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/random.h"

#include <cinttypes>

#include <array>
#include <random>
#include <stdexcept>

#include "roq/core/binascii/base64.h"

#include "roq/core/crypto/sha.h"

namespace roq {
namespace deribit {

namespace {
static std::random_device RANDOM_DEVICE;
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
constexpr auto RANDOM_BYTES = size_t{32};
}  // namespace

Random::Random(const std::string_view& secret)
    : _secret(secret) {
}

std::string Random::create_raw_data(
    const std::chrono::nanoseconds now) {
  char buffer[4096];  // XXX use array
  for (size_t i = 0; i < (RANDOM_BYTES / 4); ++i) {
    uint32_t value = DISTRIBUTION(RANDOM_DEVICE);
    *reinterpret_cast<uint32_t *>(buffer + sizeof(uint32_t) * i) = value;
  }
  auto nonce = core::binascii::Base64::encode(
      buffer,
      sizeof(uint32_t) * (RANDOM_BYTES / 4));
  auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(
      now).count();
  snprintf(  // XXX use charconv
      buffer,
      std::size(buffer),
      "%013" PRIu64 ".%s",
      msecs,
      nonce.c_str());
  return std::string(buffer, 14 + nonce.length());
}

std::string Random::create_password(const std::string_view& raw_data) {
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
