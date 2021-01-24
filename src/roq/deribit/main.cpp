/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/application.h"

namespace {
constexpr std::string_view DESCRIPTION = "Roq Deribit Gateway";
}  // namespace

int main(int argc, char **argv) {
  return roq::deribit::Application(
             argc, argv, DESCRIPTION, ROQ_BUILD_VERSION, ROQ_BUILD_TYPE, ROQ_GIT_DESCRIBE_HASH)
      .run();
}
