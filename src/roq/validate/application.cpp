/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/validate/application.h"

#include <string>
#include <vector>

#include "roq/validate/config.h"
#include "roq/validate/strategy.h"

namespace roq {
namespace deribit {
namespace validate {

int Application::main(int argc, char **argv) {
  if (argc == 1)
    throw std::runtime_error("Expected exactly one argument");
  Config config;
  std::vector<std::string> connections(
      argv + 1,
      argv + argc);
  client::Trader(config, connections).dispatch<Strategy>();
  return EXIT_SUCCESS;
}

}  // namespace validate
}  // namespace deribit
}  // namespace roq
