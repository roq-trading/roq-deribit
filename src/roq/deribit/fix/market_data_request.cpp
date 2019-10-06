/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/market_data_request.h"

#include "roq/logging.h"

#include "roq/core/fix/market_data_request.h"
#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

constexpr auto MARKET_DEPTH = size_t{20};

core::utils::Message MarketDataRequest::encode(
    core::utils::Buffer& buffer,
    uint64_t& msg_seq_num,
    std::chrono::nanoseconds sending_time) const {
  assert(symbols.empty() != symbol.empty());  // one or the other
  core::fix::Writer writer(
      buffer,
      FIX_VERSION,
      core::fix::MarketDataRequest::msg_type,
      SENDER_COMP_ID,
      TARGET_COMP_ID,
      msg_seq_num,
      sending_time);
  writer
    .write(core::fix::Field::MD_REQ_ID, md_req_id)
    .write(
        core::fix::Field::SUBSCRIPTION_REQUEST_TYPE,
        core::fix::SubscriptionRequestType::SNAPSHOT_UPDATES)
    .write(core::fix::Field::MARKET_DEPTH, MARKET_DEPTH)
    .write(core::fix::Field::MD_UPDATE_TYPE, core::fix::MDUpdateType::INCREMENTAL_REFRESH)
    .write(static_cast<uint32_t>(fix::Deribit::TRADE_AMOUNT), 0)
    .write(static_cast<uint32_t>(fix::Deribit::SINCE_TIMESTAMP), 0)
    .write(core::fix::Field::NO_MD_ENTRY_TYPES, 3)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::BID)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::OFFER)
    .write(core::fix::Field::MD_ENTRY_TYPE, core::fix::MDEntryType::TRADE);
  if (symbols.empty()) {
    writer
      .write(core::fix::Field::NO_RELATED_SYM, size_t{1})
      .write(core::fix::Field::SYMBOL, symbol);
  } else {
    writer.write(core::fix::Field::NO_RELATED_SYM, symbols.size());
    for (auto& iter : symbols)
      writer.write(core::fix::Field::SYMBOL, iter);
  }
  return writer.finish();
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
