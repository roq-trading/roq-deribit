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
    json_currencies.cpp
    json_instruments.cpp
    json_subscription.cpp
    json_ticker.cpp
    json_trades_2.cpp
    json_types.cpp
    # sbe_parser.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(
  ${TARGET_NAME}
  ${PROJECT_NAME}-fix
  ${PROJECT_NAME}-json
  ${PROJECT_NAME}-sbe
  ${PROJECT_NAME}-tools
  roq-server::roq-server
  roq-fix::roq-fix
  roq-utils::roq-utils
  roq-core::roq-core
  roq-io::roq-io
  roq-utils::roq-utils
  roq-logging::roq-logging
  aklomp::base64
  Catch2::Catch2
  ${UUID_LIBRARIES}
  ${RT_LIBRARIES})

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
