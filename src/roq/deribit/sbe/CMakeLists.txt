set(TARGET_NAME ${PROJECT_NAME}-sbe)

set(SOURCES config.cpp frame.cpp map.cpp parser.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-logging::roq-logging fmt::fmt)
