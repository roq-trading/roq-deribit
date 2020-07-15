set(TARGET_NAME "${PROJECT_NAME}-fix")

set(SOURCES
  adv_order_type.cpp
  utils.cpp
)

include(RoqAutogen)

roq_autogen(
  OUTPUT AUTOGEN_HEADERS
  PATTERN "*.json"
  NAMESPACE "roq/deribit/fix"
  OUTPUT_TYPE "fix"
  FILE_TYPE "h")

add_custom_target(autogen-fix-headers ALL
  DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT AUTOGEN_SOURCES
  PATTERN "*.json"
  NAMESPACE "roq/deribit/fix"
  OUTPUT_TYPE "fix"
  FILE_TYPE "cpp")

add_library("${TARGET_NAME}" STATIC
  ${SOURCES}
  ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-fix-headers)
