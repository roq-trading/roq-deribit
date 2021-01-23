set(TARGET_NAME "${PROJECT_NAME}")

add_subdirectory("fix")
add_subdirectory("json")

set(SOURCES
    application.cpp
    common.cpp
    config.cpp
    fix.cpp
    flags.cpp
    gateway.cpp
    random.cpp
    web_socket.cpp
    main.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS fix_state.json web_socket_state.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/deribit"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "h"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/deribit"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_executable("${TARGET_NAME}" ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-headers autogen-fix-headers autogen-json-headers)

target_link_libraries(
  "${TARGET_NAME}"
  "${PROJECT_NAME}-fix"
  "${PROJECT_NAME}-json"
  roq-server::roq-server
  roq-core::roq-core
  roq-api::roq-api
  roq-logging::roq-logging
  absl::flags
  absl::flat_hash_map
  absl::flat_hash_set
  ${LIBEVENT_LIBRARIES}
  ${UUID_LIBRARIES}
  ${RT_LIBRARIES})

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

target_compile_definitions(
  "${TARGET_NAME}"
  PRIVATE ROQ_PACKAGE_NAME="${TARGET_NAME}" ROQ_BUILD_VERSION="${GIT_REPO_VERSION}"
          ROQ_GIT_DESCRIBE_HASH="${GIT_DESCRIBE_HASH}" ROQ_BUILD_TYPE="${ROQ_BUILD_TYPE}")

install(TARGETS ${TARGET_NAME})
