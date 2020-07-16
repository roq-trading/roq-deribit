set(TARGET_NAME "${PROJECT_NAME}-json")

set(SOURCES
  parser.cpp
  utils.cpp
)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
  auth.json
  channel.json
  currencies.json
  currency.json
  direction.json
  error.json
  field.json
  instrument.json
  instruments.json
  kind.json
  method.json
  option_type.json
  order_book.json
  position.json
  positions.json
  request_type.json
  state.json
  ticker.json
)

roq_autogen(
  OUTPUT AUTOGEN_HEADERS
  NAMESPACE "roq/deribit/json"
  OUTPUT_TYPE "json"
  FILE_TYPE "h"
  SOURCES ${AUTOGEN_SCHEMAS})

add_custom_target(autogen-json-headers ALL
  DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT AUTOGEN_SOURCES
  NAMESPACE "roq/deribit/json"
  OUTPUT_TYPE "json"
  FILE_TYPE "cpp"
  SOURCES ${AUTOGEN_SCHEMAS})

add_library("${TARGET_NAME}" STATIC
  ${SOURCES}
  ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-json-headers)

target_link_libraries("${TARGET_NAME}"
  roq-core::roq-core
  roq-logging::roq-logging
  fmt::fmt)
