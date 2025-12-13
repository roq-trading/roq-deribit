/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::GetAccountSummaryAck;

TEST_CASE("failure", "[json_get_account_summary_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"get_account_summary",)"
                 R"("error":{)"
                 R"("code":-32602,)"
                 R"("data":{)"
                 R"("reason":"value required",)"
                 R"("param":"currency")"
                 R"(},)"
                 R"("message":"Invalid params")"
                 R"(},)"
                 R"("usIn":1765524680582419,)"
                 R"("usOut":1765524680582487,)"
                 R"("usDiff":68,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "get_account_summary"sv);
    CHECK(obj.error.code == -32602);
    CHECK(obj.error.message == "Invalid params"sv);
    CHECK(obj.error.data.reason == "value required"sv);
    CHECK(obj.error.data.param == "currency"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("success", "[json_get_account_summary_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"get_account_summary",)"
                 R"("result":{)"
                 R"("block_rfq_self_match_prevention":false,)"
                 R"("equity":0.0,)"
                 R"("session_upl":0.0,)"
                 R"("projected_delta_total":0.0,)"
                 R"("id":10580,)"
                 R"("disable_kyc_verification":true,)"
                 R"("additional_reserve":0.0,)"
                 R"("estimated_liquidation_ratio":0.0,)"
                 R"("margin_model":"segregated_sm",)"
                 R"("type":"main",)"
                 R"("username":"test123",)"
                 R"("options_theta":0.0,)"
                 R"("locked_balance":0.0,)"
                 R"("options_pl":0.0,)"
                 R"("self_trading_reject_mode":"reject_taker",)"
                 R"("portfolio_margining_enabled":false,)"
                 R"("total_pl":0.0,)"
                 R"("spot_reserve":0.0,)"
                 R"("session_rpl":0.0,)"
                 R"("referrer_id":null,)"
                 R"("initial_margin":0.0,)"
                 R"("projected_initial_margin":0.0,)"
                 R"("margin_balance":0.0,)"
                 R"("email":"thraneh@gmail.com",)"
                 R"("options_vega":0.0,)"
                 R"("options_session_rpl":0.0,)"
                 R"("options_vega_map":{},)"
                 R"("maintenance_margin":0.0,)"
                 R"("change_margin_model_api_limit":{"timeframe":86400000,"rate":10},)"
                 R"("futures_session_rpl":0.0,)"
                 R"("delta_total_map":{},)"
                 R"("limits":{)"
                 R"("matching_engine":{)"
                 R"("block_rfq_maker":{)"
                 R"("rate":10,)"
                 R"("burst":20)"
                 R"(},)"
                 R"("cancel_all":{"rate":5,"burst":20},)"
                 R"("guaranteed_mass_quotes":{"rate":2,"burst":2},)"
                 R"("maximum_mass_quotes":{"rate":10,"burst":10},)"
                 R"("maximum_quotes":{"rate":500,"burst":500},)"
                 R"("spot":{"rate":5,"burst":20},)"
                 R"("trading":{)"
                 R"("total":{"rate":5,"burst":20})"
                 R"(})"
                 R"(},)"
                 R"("limits_per_currency":false,)"
                 R"("non_matching_engine":{"rate":20,"burst":100}},)"
                 R"("options_delta":0.0,)"
                 R"("receive_notifications":false,)"
                 R"("projected_maintenance_margin":0.0,)"
                 R"("available_funds":0.0,)"
                 R"("fee_balance":0.0,)"
                 R"("options_session_upl":0.0,)"
                 R"("interuser_transfers_enabled":false,)"
                 R"("options_gamma":0.0,)"
                 R"("security_keys_enabled":true,)"
                 R"("futures_pl":0.0,)"
                 R"("options_theta_map":{},)"
                 R"("options_gamma_map":{},)"
                 R"("cross_collateral_enabled":false,)"
                 R"("creation_timestamp":1567103182920,)"
                 R"("options_value":0.0,)"
                 R"("delta_total":0.0,)"
                 R"("balance":0.0,)"
                 R"("estimated_liquidation_ratio_map":{},)"
                 R"("mandatory_tfa":false,)"
                 R"("system_name":"rocknroll",)"
                 R"("futures_session_upl":0.0,)"
                 R"("trading_products_details":[{)"
                 R"("enabled":true,)"
                 R"("product":"perpetual",)"
                 R"("overwriteable":false,)"
                 R"("requires_consent":false)"
                 R"(},{)"
                 R"("enabled":true,)"
                 R"("product":"futures",)"
                 R"("overwriteable":false,)"
                 R"("requires_consent":false)"
                 R"(},{)"
                 R"("enabled":true,)"
                 R"("product":"options",)"
                 R"("overwriteable":false,)"
                 R"("requires_consent":false)"
                 R"(},{)"
                 R"("enabled":true,)"
                 R"("product":"future_combos",)"
                 R"("overwriteable":false,)"
                 R"("requires_consent":false)"
                 R"(},{)"
                 R"("enabled":true,)"
                 R"("product":"option_combos",)"
                 R"("overwriteable":false,)"
                 R"("requires_consent":false)"
                 R"(},{)"
                 R"("enabled":true,)"
                 R"("product":"spots",)"
                 R"("overwriteable":false,)"
                 R"("requires_consent":false)"
                 R"(})"
                 R"(],)"
                 R"("mmp_enabled":false,)"
                 R"("currency":"XRP",)"
                 R"("self_trading_extended_to_subaccounts":false,)"
                 R"("available_withdrawal_funds":0.0)"
                 R"(},)"
                 R"("usIn":1765511740921477,)"
                 R"("usOut":1765511740924599,)"
                 R"("usDiff":3122,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "get_account_summary"sv);
    CHECK(obj.error.code == 0);
    CHECK(obj.result.maintenance_margin == 0.0_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
