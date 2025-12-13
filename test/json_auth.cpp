/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Auth;

TEST_CASE("simple", "[json_auth]") {
  auto message =
      R"({)"
      R"("jsonrpc":"2.0",)"
      R"("id":"auth",)"
      R"("result":{)"
      R"("enabled_features":[],)"
      R"("access_token":"1796961742820.1X8iNX52.zryvAz8d_EkqSKCa55krevzBMQU-e7fQi_wZ2zn1gv6gVMdpb-JdIPqkebqrB4S4v1GXRMvvn67iA655Q_4ARUjwcVns4t5Pq5tc31VvouVhLOuu0wuou3VexGNbw84un0XP2k9aJPPQyiWndYUkagPhn-qeYi7fw8r5gWY1pVCGOpRGM13IKPfvBTNFo_TURUpq1282jZ8VUCkx_4deA13QcQvQ3vQ0Jfyiz7ZyUqdh0c8JxYV7vqajhu6AUXYUCH0JDVEv0qqVQXepR6N7zIZLSZE-gfbo",)"
      R"("expires_in":31536000,)"
      R"("refresh_token":"1796961742820.1TsgfAFL.dnDPkBoPj2lRRLXzOgGxqohvQ9PxdtJvGEPcB0KFssEWdMAtuEhHoypYvAKFOtkV_XCLncQhbWDbXYN31TP68OyVvcAOcCpFXbWxzjf4mKaCuUqQ7YAm_aGRvW3jbxHQeLT3eVTGuOBkSO6ICfYn0BTfG7qZI_u12c4LgU4WKAjx15ZHrCYT8_fZ65qV-liHod3Pm_oKgpZo-HOeGlg5ta3Mm0oDsVXkpTw_w9sSSvdlm1VEV1Ix_-YoWwfL6DaUuU33wkZGtsk9ekfYprD7nNtVxVru71s",)"
      R"("scope":"block_trade:read connection trade:read_write account:read wallet:read mainaccount",)"
      R"("token_type":"bearer")"
      R"(},)"
      R"("usIn":1765425742820267,)"
      R"("usOut":1765425742820804,)"
      R"("usDiff":537,)"
      R"("testnet":false)"
      R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "auth"sv);
    CHECK(obj.result.token_type == "bearer"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
