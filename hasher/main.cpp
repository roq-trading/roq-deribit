/* Copyright (c) 2017-2021, Hans Erik Thrane */

#define __STDC_FORMAT_MACROS 1

#include <inttypes.h>
#include <stdint.h>

#include <charconv>
#include <string_view>

#include "roq/deribit/tools/hasher.h"

using namespace std::literals;

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

int main(int argc, char **argv) {
  // validate
  if (argc < 3) {
    fprintf(stderr, "ARGS: secret timestamp [nonce]\n");
    return EXIT_FAILURE;
  }

  // flags
  std::string_view secret{argv[1]};
  std::string_view timestamp{argv[2]};
  auto nonce = argc >= 4 ? std::string_view{argv[3]} : std::string_view{};

  // convert
  int64_t value = {};
  std::from_chars(std::data(timestamp), std::data(timestamp) + std::size(timestamp), value);
  std::chrono::milliseconds real_timestamp{value};

  // summary
  printf("   secret : \"%.*s\"\n", static_cast<int>(std::size(secret)), std::data(secret));
  printf("timestamp : %" PRId64 "\n", value);

  // compute
  tools::Hasher hasher(secret);
  auto real_nonce = std::empty(nonce) ? hasher.create_nonce() : std::string{nonce};
  auto [signature, used_timestamp] = hasher.create_signature(real_timestamp, real_nonce);

  // summary
  printf("    nonce : \"%.*s\"\n", static_cast<int>(std::size(real_nonce)), std::data(real_nonce));
  printf("signature : \"%.*s\"\n", static_cast<int>(std::size(signature)), std::data(signature));
  printf("timestamp : %" PRId64 "\n", used_timestamp.count());

  return EXIT_SUCCESS;
}
