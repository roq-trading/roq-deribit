/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gflags/gflags.h>

#include "roq/application.h"
#include "roq/logging.h"

// #include "roq/core/server.h"

#include "roq/deribit/conf/config.h"

#include "roq/deribit/gateway.h"

DEFINE_string(listen, "", "bind address (path)");
// DEFINE_validator(listen, ...);

DEFINE_string(config_directory, "", "config directory (path)");
DEFINE_string(config_file, "", "config file (path)");
DEFINE_string(config_variables, "", "config variables (path)");

DEFINE_string(simulation_file, "", "simulation file");

namespace {
constexpr const char *DESCRIPTION = "Roq (Gateway) Simulator";
}  // namespace

namespace {
class Application final : public roq::Application {
 public:
  using roq::Application::Application;

 protected:
  int main(int argc, char **argv) override {
    LOG(INFO) << "Parse configuration";
    roq::deribit::conf::Config config(
        FLAGS_config_directory,
        FLAGS_config_file,
        FLAGS_config_variables);
    VLOG(1) << "config=" << config;
    LOG(INFO) << "Starting the gateway";
    roq::server::Trading<roq::deribit::Gateway>(
        config,
        FLAGS_listen,
        config).dispatch();
    return EXIT_SUCCESS;
  }
};
}  // namespace

int main(int argc, char **argv) {
  return Application(argc, argv, DESCRIPTION, VERSION).run();
}
