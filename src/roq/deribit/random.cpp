/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/random.h"

#include <openssl/sha.h>

#include <cinttypes>

#include <random>
#include <stdexcept>

#include "roq/core/base64.h"

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
  auto nonce = core::b64_encode(
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
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  if (SHA256_Init(&sha256) == 1) {
    if (SHA256_Update(&sha256, raw_data.data(), raw_data.length()) == 1) {
      if (SHA256_Update(&sha256, access_secret.data(), access_secret.length()) == 1) {
        if (SHA256_Final(hash, &sha256) == 1) {
          return core::b64_encode(hash, std::size(hash));
        }
      }
    }
  }
  throw std::runtime_error("Unexpected [sha256]");
}

}  // namespace deribit
}  // namespace roq
