/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/settings.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

Settings::Settings(args::Parser const &args, server::Type type)
    : server::flags::Settings{args, type, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER}, exchange{flags::Flags::exchange()} {
  log::debug("settings={}"sv, *this);
}

}  // namespace deribit
}  // namespace roq
