/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/conf/connection.h"

namespace roq {
namespace deribit {

std::ostream& operator<<(
    std::ostream& stream,
    const Connection& value) {
  return stream << "{"
    "url=\"" << value.url << "\""
    "}";
}

}  // namespace deribit
}  // namespace roq
