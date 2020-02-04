/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/deribit/fix/md_full.h"

#include "roq/core/fix/exception.h"
#include "roq/core/fix/md_full.h"
#include "roq/core/fix/utils.h"

#include "roq/deribit/fix/deribit.h"
#include "roq/deribit/fix/utils.h"

#include "roq/logging.h"

namespace roq {
namespace deribit {
namespace fix {

namespace {
constexpr bool has_field(const auto& field) {
  return core::fix::MDFull::has_field(field);
}

template <auto field>
constexpr void check_field() {
  static_assert(has_field(field));
}

template <auto field>
constexpr void non_standard_field() {
  static_assert(has_field(field) == false);
}

bool update_field(
    auto& result,
    const auto& tag,
    const auto& field,
    const auto& value) {
  try {
    switch (field) {
      // key
      case core::fix::Field::MD_ENTRY_TYPE:
        check_field<core::fix::Field::MD_ENTRY_TYPE>();
        return false;  // break
      // standard
      case core::fix::Field::MD_ENTRY_DATE:
        check_field<core::fix::Field::MD_ENTRY_DATE>();
        core::fix::update(result.md_entry_date, value);
        break;
      case core::fix::Field::MD_ENTRY_PX:
        check_field<core::fix::Field::MD_ENTRY_PX>();
        core::fix::update(result.md_entry_px, value);
        break;
      case core::fix::Field::MD_ENTRY_SIZE:
        check_field<core::fix::Field::MD_ENTRY_SIZE>();
        core::fix::update(result.md_entry_size, value);
        break;
      case core::fix::Field::SECONDARY_ORDER_ID:
        check_field<core::fix::Field::SECONDARY_ORDER_ID>();
        core::fix::update(result.secondary_order_id, value);
        break;
      case core::fix::Field::TEXT:
        check_field<core::fix::Field::TEXT>();
        core::fix::update(result.text, value);
        break;
      // non-standard
      /*
      case core::fix::Field::MD_UPDATE_ACTION:
        non_standard_field<core::fix::Field::MD_UPDATE_ACTION>();
        return;
      */
      case core::fix::Field::ORD_STATUS:
        non_standard_field<core::fix::Field::ORD_STATUS>();
        core::fix::update(result.ord_status, value);
        break;
      case core::fix::Field::SIDE:
        non_standard_field<core::fix::Field::SIDE>();
        core::fix::update(result.side, value);
        break;
      default:
        if (has_field(field)) {
          DLOG(FATAL)(
              FMT_STRING("Unexpected tag={} field={}"),
              tag,
              field);
          break;
        }
        // deribit specific
        switch (static_cast<Deribit>(tag)) {
          case Deribit::LABEL:
            core::fix::update(result.deribit_label, value);
            break;
          case Deribit::LIQUIDATION:
            core::fix::update(result.deribit_liquidation, value);
            break;
          case Deribit::TRADE_ID:
            core::fix::update(result.deribit_trade_id, value);
            break;
          default:
            return false;
        }
    }
    return true;
  } catch (core::fix::Exception&) {
    throw;
  } catch (std::runtime_error& e) {
    throw core::fix::ParseError(tag, value, e);
  }
}
}  // namespace

MDFull::MDFull(
    core::fix::message_t::const_iterator& iter,
    const core::fix::message_t::const_iterator& end) {
  assert(iter != end);

  // XXX can we move this to Array?
  auto& [tag, value] = *iter;
  auto field = core::fix::parse_field(tag);
  static_assert(core::fix::MDFull::key_field == core::fix::Field::MD_ENTRY_TYPE);
  if (field != core::fix::MDFull::key_field)
    throw core::fix::InvalidField(tag, value);
  core::fix::update(md_entry_type, value);

  // remaining fields
  for (++iter; iter != end; ++iter) {
    auto& [tag, value] = *iter;
    auto field = core::fix::parse_field(tag);
    if (update_field(
          *this,
          tag,
          field,
          value) == false)
      break;
  }
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
