set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES map.cpp parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    auth.json
    auth_result.json
    channel.json
    chart_data.json
    chart_trades.json
    chart_trades_params_data.json
    chart_trades_params.json
    combo_state.json
    currency.json
    direction.json
    error.json
    field.json
    get_account_summary_ack_error_data.json
    get_account_summary_ack_error.json
    get_account_summary_ack.json
    get_account_summary_ack_result.json
    get_chart_data_ack.json
    get_currencies_ack.json
    get_instruments_ack.json
    get_user_trades_by_currency_ack_error_data.json
    get_user_trades_by_currency_ack_error.json
    get_user_trades_by_currency_ack.json
    get_user_trades_by_currency_ack_result.json
    instrument.json
    instrument_state.json
    instrument_state_params_data.json
    instrument_state_params.json
    instrument_type.json
    kind.json
    liquidity.json
    method.json
    option_type.json
    order_book.json
    order_state.json
    order_type.json
    platform_state.json
    platform_state_params_data.json
    platform_state_params.json
    position.json
    quote.json
    quote_params_data.json
    quote_params.json
    request_type.json
    settlement_period.json
    state.json
    subscribe_ack_error.json
    subscribe_ack.json
    ticker.json
    ticker_params_data.json
    ticker_params.json
    tick_size_step.json
    time_in_force.json
    trade.json
    user_changes.json
    user_changes_params_data.json
    user_changes_params.json
    user_orders.json
    user_orders_params_data.json
    user_orders_params.json
    user_portfolio.json
    user_portfolio_params_data.json
    user_portfolio_params.json
    user_trades.json
    user_trades_params.json)

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

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-logging::roq-logging fmt::fmt)
