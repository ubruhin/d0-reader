idf_component_register(
    SRCS
    "Atm90e26.cpp"
    "Buzzer.cpp"
    "Led.cpp"
    "main.cpp"
    "MeasureThread.cpp"
    "OcppConnection.cpp"
    "Relay.cpp"
    "Webinterface.cpp"
    "Wifi.cpp"
    INCLUDE_DIRS "."
)

target_compile_options(${COMPONENT_TARGET} PUBLIC -Wno-error -Wall -Wextra)

if(BUILD_STRICT)
target_compile_options(${COMPONENT_TARGET} PUBLIC -Werror)
endif()
