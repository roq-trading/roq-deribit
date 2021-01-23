/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/application.h"

#include "roq/deribit/config.h"
#include "roq/deribit/flags.h"
#include "roq/deribit/gateway.h"

namespace roq {
namespace deribit {

int Application::main(int, char **) {
  LOG(INFO)("Parse configuration");
  Config config(Flags::config_file());
  VLOG(1)(R"(config={})", config);
  LOG(INFO)("Starting the gateway...");
  roq::server::Trading<Gateway>(
      ROQ_PACKAGE_NAME, config, server::RequestIdType::SEQUENTIAL, config)
      .dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
