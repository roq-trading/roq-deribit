set(TARGET_NAME ${PROJECT_NAME}-benchmark)

set(SOURCES
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
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-fix ${PROJECT_NAME}-json roq-server::roq-server roq-fix::roq-fix benchmark::benchmark)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

install(TARGETS ${TARGET_NAME})

add_custom_target(
  benchmark
  COMMAND ${TARGET_NAME}
  DEPENDS ${TARGET_NAME}
  VERBATIM)
