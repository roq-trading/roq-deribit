/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/test_request.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::TestRequest::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

void update_field(
    auto& result,
    auto& iter) {
  auto& [tag, value] = *iter;
  try {
    auto field = core::fix::parse_field(tag);
    switch (field) {
      case core::fix::Field::TEST_REQ_ID:
        check_field<core::fix::Field::TEST_REQ_ID>();
        core::fix::update(result.test_req_id, value);
        break;
      default:
        if (core::fix::TestRequest::has_field(field))
          break;
        throw core::fix::InvalidField(tag, value);
    }
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

TestRequest TestRequest::create(const core::fix::message_t& message) {
  TestRequest result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}

core::utils::Message TestRequest::encode(
    core::fix::Writer& writer) const {
  return writer
    .write(
        core::fix::Field::TEST_REQ_ID,
        test_req_id)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
