cmake_minimum_required(VERSION 3.25)
project(etac)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-O3)

find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

# 1) Setup ANTLR4 (local source)
set(ANTLR4_INSTALL OFF CACHE BOOL "Don't install ANTLR4")
set(WITH_DEMO OFF CACHE BOOL "Don't build ANTLR4 demos")
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Don't build ANTLR4 tests")
add_subdirectory(libs/antlr4/runtime/Cpp)
add_subdirectory(src)

add_custom_target(eta_runtime ALL
    COMMAND ${MAKE_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/runtime
    COMMENT "Building Eta runtime"
    USES_TERMINAL
)

add_executable(etac
    src/main.cpp
)
add_dependencies(etac eta_runtime)

target_link_libraries(etac_backend PUBLIC
    etac_frontend
)

target_link_libraries(etac PRIVATE
    etac_backend
)
