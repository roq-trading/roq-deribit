/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/conf/account.h"

#include "roq/stream.h"

namespace roq {
namespace deribit {
namespace conf {

std::ostream& operator<<(
    std::ostream& stream,
    const Account& value) {
  return stream << "{"
    "account=" << value.account << ", "
    "broker=\"" << value.broker << "\", "
    "seat_no=" << value.seat_no <<
    "}";
}

}  // namespace conf
}  // namespace deribit
}  // namespace roq
