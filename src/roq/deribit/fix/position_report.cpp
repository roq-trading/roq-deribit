/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/position_report.h"

#include "roq/core/charconv.h"

#include "roq/core/fix/array.h"
#include "roq/core/fix/exception.h"
#include "roq/core/fix/position_report.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::PositionReport::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

void update(
    auto& result,
    auto&& iter,
    const auto& end,
    auto& buffer) {
  while (iter != end) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    try {
      switch (field) {
        case core::fix::Field::NO_POSITIONS: {
          check_field<core::fix::Field::NO_POSITIONS>();
          result.positions =
            core::fix::Array<decltype(result.positions)>::create(
                buffer,
                iter,
                end);
          continue;  // note!
        }
        case core::fix::Field::POS_MAINT_RPT_ID:
          check_field<core::fix::Field::POS_MAINT_RPT_ID>();
          core::fix::update(result.pos_maint_rpt_id, value);
          break;
        case core::fix::Field::POS_REQ_ID:
          check_field<core::fix::Field::POS_REQ_ID>();
          core::fix::update(result.pos_req_id, value);
          break;
        case core::fix::Field::POS_REQ_RESULT:
          check_field<core::fix::Field::POS_REQ_RESULT>();
          core::fix::update(result.pos_req_result, value);
          break;
        case core::fix::Field::POS_REQ_TYPE:
          check_field<core::fix::Field::POS_REQ_TYPE>();
          core::fix::update(result.pos_req_type, value);
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
    ++iter;
  }
}
}  // namespace

PositionReport PositionReport::create(
    const core::fix::message_t& message,
    core::fix::Buffer& buffer) {
  PositionReport result;
  update(
      result,
      message.begin(),
      message.end(),
      buffer);
  return result;
}
}  // namespace fix
}  // namespace deribit
}  // namespace roq
