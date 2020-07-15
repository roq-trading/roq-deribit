set(TARGET_NAME "${PROJECT_NAME}")

add_subdirectory("fix")
add_subdirectory("json")

set(SOURCES
  application.cpp
  common.cpp
  config.cpp
  fix.cpp
  gateway.cpp
  main.cpp
  options.cpp
  random.cpp
  web_socket.cpp
)

include(RoqAutogen)

roq_autogen(
  OUTPUT AUTOGEN_HEADERS
  PATTERN "*.json"
  NAMESPACE "roq/deribit"
  OUTPUT_TYPE "state"
  FILE_TYPE "h")

add_custom_target(autogen-headers ALL
  DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT AUTOGEN_SOURCES
  PATTERN "*.json"
  NAMESPACE "roq/deribit"
  OUTPUT_TYPE "state"
  FILE_TYPE "cpp")

add_executable("${TARGET_NAME}"
  ${SOURCES}
  ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}"
  autogen-headers
  autogen-fix-headers
  autogen-json-headers)

target_link_libraries("${TARGET_NAME}"
  "${PROJECT_NAME}-fix"
  "${PROJECT_NAME}-json"
  roq-server
  roq-core
  roq-api::roq-api
  roq-logging
  absl::flat_hash_map
  absl::flat_hash_set
  gflags
  OpenSSL::SSL
  ${HTTP_PARSER_LIBRARIES}
  ${BROTLI_LIBRARIES}
  ${RT_LIBRARIES}
)

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

target_compile_definitions("${TARGET_NAME}" PRIVATE
  PACKAGE_NAME="${TARGET_NAME}"
  VERSION="${GIT_REPO_VERSION}")

install(TARGETS ${TARGET_NAME})
