/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/application.h"

namespace {
constexpr std::string_view DESCRIPTION = "Roq Deribit Gateway";
}  // namespace

int main(int argc, char **argv) {
  return roq::deribit::Application(
      argc,
      argv,
      DESCRIPTION,
      VERSION).run();
}
