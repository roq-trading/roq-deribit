/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/logging.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  roq::Logger::initialize();
  auto result = RUN_ALL_TESTS();
  roq::Logger::shutdown();
  return result;
}
