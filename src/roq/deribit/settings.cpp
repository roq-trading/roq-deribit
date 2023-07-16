/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/settings.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

Settings::Settings(args::Parser const &args)
    : server::flags::Settings{args, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER}, flags::Flags{flags::Flags::create()},
      common{flags::Common::create()}, fix{flags::FIX::create()}, ws{flags::WS::create()},
      multicast{flags::Multicast::create()} {
  log::debug("settings={}"sv, *this);
}

}  // namespace deribit
}  // namespace roq
