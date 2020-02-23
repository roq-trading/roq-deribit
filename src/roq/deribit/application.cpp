/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/application.h"

#include "roq/deribit/config.h"
#include "roq/deribit/gateway.h"
#include "roq/deribit/options.h"

namespace roq {
namespace deribit {

int Application::main(int, char **) {
  LOG(INFO)("Parse configuration");
  Config config(FLAGS_config_file);
  VLOG(1)(FMT_STRING("config={}"), config);
  LOG(INFO)("Starting the gateway");
  roq::server::Trading<Gateway>(
      config,
      FLAGS_listen,
      server::RequestIdType::SEQUENTIAL,
      config).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace deribit
}  // namespace roq
