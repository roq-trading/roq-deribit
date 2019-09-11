/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/random.h"

#include <cinttypes>

#include <random>
#include <stdexcept>

#include "roq/core/base64/base64.h"

#include "roq/core/crypto/sha.h"

namespace roq {
namespace deribit {

namespace {
static std::random_device RANDOM_DEVICE;
static std::uniform_int_distribution<uint32_t> DISTRIBUTION;
constexpr size_t RANDOM_BYTES = 32;
}  // namespace

std::string Random::create_raw_data(
    const std::chrono::nanoseconds now) {
  char buffer[4096];
  for (size_t i = 0; i < (RANDOM_BYTES / 4); ++i) {
    uint32_t value = DISTRIBUTION(RANDOM_DEVICE);
    *reinterpret_cast<uint32_t *>(buffer + sizeof(uint32_t) * i) = value;
  }
  auto nonce = core::base64::encode(
      buffer,
      sizeof(uint32_t) * (RANDOM_BYTES / 4));
  auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(
      now).count();
  snprintf(
      buffer,
      std::size(buffer),
      "%013" PRIu64 ".%s",
      msecs,
      nonce.c_str());
  return std::string(buffer, 14 + nonce.length());
}

std::string Random::create_password(
    const std::string_view& raw_data,
    const std::string_view& access_secret) {
  core::crypto::SHA256 sha;
  sha.update(raw_data);
  sha.update(access_secret);
  char buffer[core::crypto::SHA256::DIGEST_LENGTH];
  auto length = sha.digest(buffer, std::size(buffer));
  return core::base64::encode(buffer, length);
}

}  // namespace deribit
}  // namespace roq
