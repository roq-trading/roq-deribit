set(TARGET_NAME "${PROJECT_NAME}-benchmark")

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
  main.cpp
)

add_executable("${TARGET_NAME}"
  ${SOURCES}
)

target_link_libraries("${TARGET_NAME}"
  "${PROJECT_NAME}-fix"
  "${PROJECT_NAME}-json"
  roq-core
  roq-logging
  fmt::fmt
  benchmark::benchmark
  ${RT_LIBRARIES}
)

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

install(TARGETS ${TARGET_NAME})
