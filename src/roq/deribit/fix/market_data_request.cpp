/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_request.h"

#include "roq/logging.h"

#include "roq/core/fix/market_data_request.h"
#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

core::utils::Message MarketDataRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time,
    const std::string_view& md_req_id,
    const std::string_view& symbol) {
  return core::fix::Writer(
      buffer,
      FIX_VERSION,
      core::fix::MarketDataRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time)
    .write(core::fix::Field::SYMBOL, symbol)
    .write(core::fix::Field::MD_REQ_ID, md_req_id)
    // TODO(thraneh): ... do not hardcode!
    .write(
        core::fix::Field::SUBSCRIPTION_REQUEST_TYPE,
        core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES)
    .write(core::fix::Field::MARKET_DEPTH, uint8_t{20})
    .write(core::fix::Field::MD_UPDATE_TYPE, core::fix::MDUpdateType::INCREMENTAL_REFRESH)
    .write(static_cast<uint32_t>(fix::Deribit::TRADE_AMOUNT), uint32_t{0})
    .write(static_cast<uint32_t>(fix::Deribit::SINCE_TIMESTAMP), uint32_t{0})
    .write(core::fix::Field::NO_MD_ENTRY_TYPES, uint8_t{3})
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::BID)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::OFFER)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::TRADE)
    .finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
