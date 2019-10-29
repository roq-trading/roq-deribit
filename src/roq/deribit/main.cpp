/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gflags/gflags.h>

#include "roq/application.h"
#include "roq/logging.h"

#include "roq/deribit/config.h"
#include "roq/deribit/gateway.h"

DEFINE_string(listen,
    "",
    "bind address (path)");
// DEFINE_validator(listen, ...);

DEFINE_string(config_file,
    "",
    "config file (path)");

namespace {
constexpr const char *DESCRIPTION = "Roq Deribit Gateway";
}  // namespace

namespace {
class Application final : public roq::Application {
 public:
  using roq::Application::Application;

 protected:
  int main(int, char **) override {
    LOG(INFO)("Parse configuration");
    roq::deribit::Config config(FLAGS_config_file);
    VLOG(1)("config={}", config);
    LOG(INFO)("Starting the gateway");
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
