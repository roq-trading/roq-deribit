/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/fill.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/fills.h"
#include "roq/core/fix/fills_grp.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const core::fix::Field& field) {
  return core::fix::FillsGrp::has_field(field);
}

bool update(
    auto& result,
    const auto& tag,
    const auto& field,
    const auto& value) {
  try {
    switch (field) {
      // key
      case core::fix::Field::FILL_EXEC_ID:
        static_assert(has_field(core::fix::Field::FILL_EXEC_ID));
        return false;  // break
      // standard
      case core::fix::Field::FILL_PX:
        static_assert(has_field(core::fix::Field::FILL_PX));
        core::fix::update(result.fill_px, value);
        break;
      case core::fix::Field::FILL_QTY:
        static_assert(has_field(core::fix::Field::FILL_QTY));
        core::fix::update(result.fill_qty, value);
        break;
      case core::fix::Field::FILL_LIQUIDITY_IND:
        static_assert(has_field(core::fix::Field::FILL_LIQUIDITY_IND));
        core::fix::update(result.fill_liquidity_ind, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)("Unexpected tag={} field={}", tag, field);
          break;
        }
        return false;
    }
    return true;
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

Fill::Fill(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);

  // XXX can we move this to Array?
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  static_assert(core::fix::Fills::key_field == core::fix::Field::FILL_EXEC_ID);
  if (field != core::fix::Fills::key_field)
    throw core::fix::InvalidField(tag, value);
  core::fix::update(fill_exec_id, value);

  // remaining fields
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    if (update(*this, tag, field, value) == false)
      return;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
