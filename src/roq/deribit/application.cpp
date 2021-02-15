/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/deribit/application.h"

#include "roq/deribit/config.h"
#include "roq/deribit/flags.h"
#include "roq/deribit/gateway.h"

using namespace roq::literals;

namespace roq {
namespace deribit {

int Application::main(int, char **) {
  LOG(INFO)(R"(Parse config_file="{}")"_sv, Flags::config_file());
  Config config(Flags::config_file());
  VLOG(1)(R"(config={})"_sv, config);
  LOG(INFO)("Starting the gateway..."_sv);
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, config, server::RequestIdType::SEQUENTIAL, config)
      .dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
