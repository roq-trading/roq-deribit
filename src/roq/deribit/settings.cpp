/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

Settings Settings::create(server::Type type) {
  auto settings = server::create_settings(type, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER);
  return {settings};
}

}  // namespace deribit
}  // namespace roq
