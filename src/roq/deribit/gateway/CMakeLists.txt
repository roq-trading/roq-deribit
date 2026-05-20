set(TARGET_NAME ${PROJECT_NAME}-gateway)

set(SOURCES
    account.cpp
    api.cpp
    channel.cpp
    config.cpp
    controller.cpp
    drop_copy.cpp
    instrument.cpp
    market_data.cpp
    order_entry.cpp
    rest.cpp
    shared.cpp
    udp_events.cpp
    udp_snapshot.cpp
    web_socket.cpp)

add_library(${TARGET_NAME} SHARED ${SOURCES})

add_dependencies(${TARGET_NAME} flags-autogen-headers autogen-fix-headers autogen-json-headers)

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_NAME}-fix
          ${PROJECT_NAME}-flags
          ${PROJECT_NAME}-json
          ${PROJECT_NAME}-sbe
          ${PROJECT_NAME}-tools
          roq-api::roq-api
          roq-core::roq-core
          roq-codec::roq-codec
          roq-fix::roq-fix
          roq-flags::roq-flags
          roq-io::roq-io
          roq-logging::roq-logging
          roq-logging::roq-logging-flags
          roq-market::roq-market
          roq-server::roq-server
          roq-server::roq-server-flags
          roq-utils::roq-utils
          roq-web::roq-web
          fmt::fmt
          unordered_dense::unordered_dense)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")

install(TARGETS ${TARGET_NAME})
