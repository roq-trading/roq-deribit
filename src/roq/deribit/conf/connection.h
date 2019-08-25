/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <iostream>
#include <string>

namespace roq {
namespace deribit {

struct Connection final {
  std::string url;
};

std::ostream& operator<<(std::ostream&, const Connection&);

}  // namespace deribit
}  // namespace roq
