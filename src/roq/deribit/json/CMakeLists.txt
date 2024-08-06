set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES map.cpp parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    auth.json
    changes.json
    channel.json
    combo_state.json
    currencies.json
    currency.json
    direction.json
    error.json
    field.json
    instrument.json
    instruments.json
    instrument_state.json
    instrument_type.json
    kind.json
    liquidity.json
    method.json
    option_type.json
    order_book.json
    order.json
    orders.json
    order_state.json
    order_type.json
    platform_state.json
    portfolio.json
    position.json
    positions.json
    quote.json
    request_type.json
    state.json
    ticker.json
    time_in_force.json
    trade.json
    trades_2.json
    trades.json)

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen_cpp(
  OUTPUT
  AUTOGEN_SOURCES
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} OBJECT ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} autogen-json-headers)

target_link_libraries(${TARGET_NAME} roq-core::roq-core roq-logging::roq-logging fmt::fmt)
