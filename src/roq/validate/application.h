/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/application.h"

namespace roq {
namespace deribit {
namespace validate {

class Application final : public roq::Application {
 public:
  using roq::Application::Application;

 protected:
  int main(int argc, char **argv) override;
};

}  // namespace validate
}  // namespace deribit
}  // namespace roq
