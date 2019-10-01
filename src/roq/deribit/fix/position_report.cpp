/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/position_report.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/position_report.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/array.h"
#include "roq/deribit/fix/buffer.h"
#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

PositionReport PositionReport::parse(
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  PositionReport result;
  parse(result, message, buffer);
  return result;
}

void PositionReport::parse(
    PositionReport& result,
    const core::fix::message_t& message,
    std::vector<std::byte>& buffer) {
  new (&result) std::remove_reference<decltype(result)>::type {};
  result.parse(message.begin(), message.end(), buffer);
}

void PositionReport::parse(
    core::fix::message_t::const_iterator&& iter,
    const core::fix::message_t::const_iterator& end,
    std::vector<std::byte>& buffer) {
  Buffer buffer_(buffer);
  while (iter != end) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    try {
      switch (field) {
        case core::fix::Field::NO_POSITIONS: {
          static_assert(core::fix::PositionReport::has_field(core::fix::Field::NO_POSITIONS));
          auto length = core::charconv::from_string<uint32_t>(value);
          ++iter;
          Array array(buffer_, positions);
          for (uint32_t i = 0; i < length; ++i) {
            if (iter == end)
              throw core::fix::UnexpectedEndOfMessage(
                  "PositionReport|PositionQty");
            auto& item = array.next();
            item.parse(iter, end);
            ++array;
          }
          if (positions.length != length)
            throw core::fix::InvalidGroupLength(
                "PositionReport|PositionQty: "
                "Invalid group length: parsed={}, expected={}",
                positions.length, length);
          continue;
        }
        case core::fix::Field::POS_MAINT_RPT_ID:
          static_assert(core::fix::PositionReport::has_field(core::fix::Field::POS_MAINT_RPT_ID));
          core::fix::update(pos_maint_rpt_id, value);
          break;
        case core::fix::Field::POS_REQ_ID:
          static_assert(core::fix::PositionReport::has_field(core::fix::Field::POS_REQ_ID));
          core::fix::update(pos_req_id, value);
          break;
        case core::fix::Field::POS_REQ_RESULT:
          static_assert(core::fix::PositionReport::has_field(core::fix::Field::POS_REQ_RESULT));
          core::fix::update(pos_req_result, value);
          break;
        case core::fix::Field::POS_REQ_TYPE:
          static_assert(core::fix::PositionReport::has_field(core::fix::Field::POS_REQ_TYPE));
          core::fix::update(pos_req_type, value);
          break;
        default:
          if (core::fix::PositionReport::has_field(field))
            break;
          throw core::fix::InvalidField(
              "PositionReport: "
              "Unexpected field={}", tag);
      }
    } catch (core::fix::Exception&) {
      throw;
    } catch (std::runtime_error& e) {
      throw core::fix::ParseError(
          "PositionReport: "
          "Parse error: "
          "field={}, value=\"{}\", what=\"{}\"",
          tag, value, e.what());
    }
    ++iter;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
