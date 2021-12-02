/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/application.h"

#include "roq/deribit/config.h"
#include "roq/deribit/flags.h"
#include "roq/deribit/gateway.h"

using namespace std::literals;

namespace roq {
namespace deribit {

int Application::main(int, char **) {
  log::info(R"(Parse config_file="{}")"sv, Flags::config_file());
  Config config(Flags::config_file(), Flags::secrets_file());
  log::info<1>("config={}"sv, config);
  log::info("Starting the gateway..."sv);
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, config).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
