set(TARGET_NAME ${PROJECT_NAME})

add_subdirectory(fix)
add_subdirectory(json)
add_subdirectory(flags)
add_subdirectory(tools)
add_subdirectory(sbe)

add_subdirectory(dump)
add_subdirectory(filter)

set(SOURCES
    account.cpp
    api.cpp
    application.cpp
    channel.cpp
    config.cpp
    drop_copy.cpp
    gateway.cpp
    instrument.cpp
    market_data.cpp
    order_entry.cpp
    rest.cpp
    settings.cpp
    shared.cpp
    udp_events.cpp
    udp_snapshot.cpp
    web_socket.cpp
    main.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS market_data_state.json order_entry_state.json web_socket_state.json drop_copy_state.json)

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "state")

add_custom_target(autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS})

add_executable(${TARGET_NAME} ${SOURCES})

add_dependencies(${TARGET_NAME} autogen-headers autogen-fix-headers autogen-json-headers)

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_NAME}-fix
          ${PROJECT_NAME}-json
          ${PROJECT_NAME}-flags
          ${PROJECT_NAME}-tools
          ${PROJECT_NAME}-sbe
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
          unordered_dense::unordered_dense)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

target_compile_definitions(
  ${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${TARGET_NAME}" ROQ_HOST="${ROQ_HOST}" ROQ_BUILD_VERSION="${GIT_REPO_VERSION}"
                         ROQ_GIT_DESCRIBE_HASH="${GIT_DESCRIBE_HASH}" ROQ_BUILD_NUMBER="${ROQ_BUILD_NUMBER}" ROQ_BUILD_TYPE="${ROQ_BUILD_TYPE}")

install(TARGETS ${TARGET_NAME})

if(BUILD_DOCS)

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}
    FILES_MATCHING
    PATTERN "*.rst*"
    PATTERN "CMakeFiles" EXCLUDE)

endif()
