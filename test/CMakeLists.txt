include(RoqAutogen)

set(TARGET_NAME ${PROJECT_NAME}-test)

set(SOURCES
    crypto.cpp
    fix_execution_report.cpp
    fix_heartbeat.cpp
    fix_logon.cpp
    fix_logout.cpp
    fix_market_data_incremental_refresh.cpp
    fix_market_data_request_reject.cpp
    fix_market_data_snapshot_full_refresh.cpp
    fix_new_order_single.cpp
    fix_order_cancel_reject.cpp
    fix_order_cancel_replace_request.cpp
    fix_order_cancel_request.cpp
    fix_position_report.cpp
    fix_reject.cpp
    fix_security_list.cpp
    fix_user_response.cpp
    json_auth.cpp
    json_get_account_summary_ack.cpp
    json_get_chart_data_ack.cpp
    json_get_currencies_ack.cpp
    json_get_instruments_ack.cpp
    json_get_user_trades_by_currency_ack.cpp
    json_platform_state.cpp
    json_quote.cpp
    json_subscribe_ack.cpp
    json_ticker.cpp
    json_types.cpp
    json_user_changes.cpp
    json_user_orders.cpp
    json_user_portfolio.cpp
    json_user_trades.cpp
    sbe_parser.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_NAME}-fix
          ${PROJECT_NAME}-json
          ${PROJECT_NAME}-sbe
          ${PROJECT_NAME}-tools
          roq-server::roq-server
          roq-fix::roq-fix
          Catch2::Catch2)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
