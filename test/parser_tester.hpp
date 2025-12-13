/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/json/parser.hpp"

namespace roq {
namespace deribit {

template <typename T>
struct ParserTester final : public json::Parser::Handler {
  using value_type = std::remove_cvref_t<T>;
  using callback_type = std::function<void(value_type const &)>;

  static void dispatch(callback_type const &callback, std::string_view const &message, size_t buffer_size, size_t max_depth) {
    core::json::BufferStack buffers{buffer_size, max_depth};
    // simple
    // XXX FIXME TODO catch2 block ???
    T obj{message, buffers};
    callback(obj);
    // parser
    // XXX FIXME TODO catch2 block ???
    ParserTester handler{callback};
    auto res = json::Parser::dispatch(handler, message, buffers, {}, false);
    CHECK(res == true);
    CHECK(handler.found_ == true);
  }

 protected:
  explicit ParserTester(callback_type const &callback) : callback_{callback} {}

  void operator()(Trace<json::Auth> const &event) override { dispatch_helper(event); }

  void operator()(Trace<json::SubscribeAck> const &event) override { dispatch_helper(event); }

  void operator()(Trace<json::PlatformState> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::InstrumentState> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::Quote> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::Ticker> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::ChartTrades> const &event, [[maybe_unused]] std::string_view const &symbol, [[maybe_unused]] uint32_t interval) override {
    dispatch_helper(event);
  }

  void operator()(Trace<json::UserPortfolio> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::UserChanges> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::UserOrders> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::UserTrades> const &event) override { dispatch_helper(event); }

  void operator()(Trace<json::GetAccountSummaryAck> const &event) override { dispatch_helper(event); }
  void operator()(Trace<json::GetUserTradesByCurrencyAck> const &event) override { dispatch_helper(event); }

  template <typename U>
  void dispatch_helper(Trace<U> const &event) {
    if constexpr (std::is_invocable_v<callback_type, U>) {
      found_ = true;
      callback_(event);
    } else {
      FAIL();
    }
  }

 private:
  callback_type const callback_;
  bool found_ = false;
};

}  // namespace deribit
}  // namespace roq
