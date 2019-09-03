/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct ExecutionReport final {
  std::string_view text;
/*
1 ClOrdID Yes Deribit replaces this field with the own value assigned by the server (it is not the client id from New Order Single)
41  OrigClOrdId Yes The original value assigned by the client in the New Order Single(D) message
39  OrdStatus Yes 0 = New, 1 = Partially filled, 2 = Filled, 8 = Rejected
54  Side  Yes 1 = Buy, 2 = Sell
60  TransactTime  Yes Time the transaction represented by this Execution Report occurred. Fix timestamp.
151 LeavesQty Yes Order quantity open for further execution (LeavesQty = OrderQty - CumQty) in Contract units corresponding to the ContractMultiplier in SecurityList
14  CumQty  Yes Total executed quantity or 0.0 in Contract units corresponding to the ContractMultiplier in SecurityList
38  OrderQty  Yes Order quantity in Contract units corresponding to the ContractMultiplier in SecurityList
5127  ConditionTriggerMethod  No  Trigger for a stop order 1 = Mark Price, 2 = Last Price, 3 = corresponding Index Price
40  OrdType Yes 1 = Market, 2 = Limit, 4 = Stop Limit, S = Stop Market
44  Price No  Price, maybe be absent for Market and Stop Market orders
18  ExecInst  No  Currently is used to mark POST ONLY orders: 6 = "Participate don't initiate", and REDUCE ONLY orders: E = " Do not increase - DNI"
99  StopPx  No  Stop price for stop limit orders
103 OrdRejReason  Yes 
58  Text  No  Free format text string, usually exceptions
207 SecurityExchange  No  "Deribit"
55  Symbol  Yes Instrument symbol
854 QtyType No  Type of quantity specified in a quantity. Currently only 1 - Contracts.
231 ContractMultiplier  No  Specifies a multiply factor to convert from contracts to total units
6 AvgPx No  Average execution price or 0.0 if not executed yet or rejected
210 MaxShow No  Maximum quantity (e.g. number of shares) within an order to be shown to other customers
100012  DeribitAdvOrderType No  if it is present then it denotes advanced order for options: 0 = Implied Volatility Order, 1 = USD Order
1188  Volatility  No  volatility for Implied Volatility Orders (options orders with fixed volatility)
839 PeggedPrice No
*/

  static ExecutionReport parse(const core::fix::message_t& message);
  static void parse(ExecutionReport&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::ExecutionReport> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::ExecutionReport& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "text=\"{}\""
        "}}",
        value.text);
  }
};
