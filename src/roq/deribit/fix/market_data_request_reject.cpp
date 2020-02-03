/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_request_reject.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/market_data_request_reject.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::MarketDataRequestReject::has_field(field);
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
      case core::fix::Field::MD_REQ_ID:
        check_field<core::fix::Field::MD_REQ_ID>();
        core::fix::update(result.md_req_id, value);
        break;
      case core::fix::Field::MD_REQ_REJ_REASON:
        check_field<core::fix::Field::MD_REQ_REJ_REASON>();
        core::fix::update(result.md_req_rej_reason, value);
        break;
    case core::fix::Field::TEXT:
        check_field<core::fix::Field::TEXT>();
      core::fix::update(result.text, value);
      break;
    default:
      if (has_field(field)) {
        DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
        break;
      }
      DLOG(FATAL)("Unknown tag={} field={}", tag, field);
      throw core::fix::InvalidField(tag, value);
    }
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

MarketDataRequestReject MarketDataRequestReject::create(
    const core::fix::message_t& message) {
  MarketDataRequestReject result;
  for (auto iter = message.begin(); iter != message.end(); ++iter)
    update_field(result, iter);
  return result;
}


}  // namespace fix
}  // namespace deribit
}  // namespace roq
