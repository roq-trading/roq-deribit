/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/position_report.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/position_report.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

PositionReport PositionReport::parse(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  PositionReport result;
  parse(result, message, buffer);
  return result;
}

void PositionReport::parse(
    PositionReport& result,
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::PositionReport::has_field(field);
}
}  // namespace

void PositionReport::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    core::fix::Buffer& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    try {
      switch (field) {
        case core::fix::Field::NO_POSITIONS: {
          static_assert(has_field(core::fix::Field::NO_POSITIONS));
          auto length = core::from_chars<uint32_t>(value);
          if (length) {
            ++iter;
            if (iter == end)
              throw core::fix::UnexpectedEndOfMessage();
            core::fix::Array array(buffer, positions, length);
            for (auto& item : array)
              item.parse(iter, end);
            continue;
          }
          break;
        }
        case core::fix::Field::POS_MAINT_RPT_ID:
          static_assert(has_field(core::fix::Field::POS_MAINT_RPT_ID));
          core::fix::update(pos_maint_rpt_id, value);
          break;
        case core::fix::Field::POS_REQ_ID:
          static_assert(has_field(core::fix::Field::POS_REQ_ID));
          core::fix::update(pos_req_id, value);
          break;
        case core::fix::Field::POS_REQ_RESULT:
          static_assert(has_field(core::fix::Field::POS_REQ_RESULT));
          core::fix::update(pos_req_result, value);
          break;
        case core::fix::Field::POS_REQ_TYPE:
          static_assert(has_field(core::fix::Field::POS_REQ_TYPE));
          core::fix::update(pos_req_type, value);
          break;
        default:
          if (has_field(field))
            break;
          throw core::fix::InvalidField(tag, value);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(tag, value, e);
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
