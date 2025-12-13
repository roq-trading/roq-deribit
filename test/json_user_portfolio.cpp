/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::UserPortfolio;

TEST_CASE("simple", "[json_user_portfolio]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"user.portfolio.btc",)"
                 R"("data":{)"
                 R"("equity":10.00295059,)"
                 R"("session_upl":0.0,)"
                 R"("projected_delta_total":0.0,)"
                 R"("additional_reserve":0.0,)"
                 R"("estimated_liquidation_ratio":0.0,)"
                 R"("margin_model":"segregated_sm",)"
                 R"("options_theta":0.0,)"
                 R"("locked_balance":0.0,)"
                 R"("options_pl":0.0,)"
                 R"("portfolio_margining_enabled":false,)"
                 R"("total_pl":0.0,)"
                 R"("spot_reserve":0.0,)"
                 R"("session_rpl":-4.0e-7,)"
                 R"("initial_margin":0.0,)"
                 R"("projected_initial_margin":0.0,)"
                 R"("margin_balance":10.00295059,)"
                 R"("options_vega":0.0,)"
                 R"("options_session_rpl":0.0,)"
                 R"("options_vega_map":{},)"
                 R"("maintenance_margin":0.0,)"
                 R"("futures_session_rpl":-4.0e-7,)"
                 R"("delta_total_map":{)"
                 R"("btc_usd":0.0)"
                 R"(},)"
                 R"("options_delta":0.0,)"
                 R"("projected_maintenance_margin":0.0,)"
                 R"("available_funds":10.00295059,)"
                 R"("fee_balance":0.0,)"
                 R"("options_session_upl":0.0,)"
                 R"("options_gamma":0.0,)"
                 R"("futures_pl":0.0,)"
                 R"("options_theta_map":{},)"
                 R"("options_gamma_map":{},)"
                 R"("cross_collateral_enabled":false,)"
                 R"("options_value":0.0,)"
                 R"("delta_total":0.0,)"
                 R"("balance":10.00295099,)"
                 R"("estimated_liquidation_ratio_map":{)"
                 R"("btc_usd":0.0)"
                 R"(},)"
                 R"("futures_session_upl":0.0,)"
                 R"("currency":"BTC",)"
                 R"("available_withdrawal_funds":10.00295058)"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "user.portfolio.btc"sv);
    auto &data = obj.params.data;
    CHECK(data.equity == 10.00295059_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
