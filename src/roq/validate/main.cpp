/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/validate/application.h"

namespace {
constexpr std::string_view DESCRIPTION = "Roq Deribit Validator";
}  // namespace

int main(int argc, char **argv) {
  return roq::deribit::validate::Application(
      argc,
      argv,
      DESCRIPTION).run();
}
