idf_component_register(
    SRCS
    "main.cpp"
    INCLUDE_DIRS "."
)

target_compile_options(${COMPONENT_TARGET} PUBLIC -Wno-error -Wall -Wextra)

if(BUILD_STRICT)
target_compile_options(${COMPONENT_TARGET} PUBLIC -Werror)
endif()
