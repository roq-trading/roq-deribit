set(TARGET_NAME "${PROJECT_NAME}-flags")

include(RoqAutogen)

set(AUTOGEN_SCHEMAS common.json config.json fix.json web_socket.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/deribit/flags"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "h"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(flags-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/deribit/flags"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

set(GITIGNORE "${CMAKE_CURRENT_SOURCE_DIR}/.gitignore")

file(WRITE "${GITIGNORE}" "")

foreach(path ${AUTOGEN_SCHEMAS})
  get_filename_component(name ${path} NAME_WE)
  file(APPEND "${GITIGNORE}" "${name}.h\n${name}.cpp\n")
endforeach()

add_library("${TARGET_NAME}" STATIC ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" flags-autogen-headers)

target_link_libraries("${TARGET_NAME}" absl::flags)

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_14)
