cmake_minimum_required(VERSION 3.10)
project(Compiler LANGUAGES CXX C)

# set the compile standard, with output compile commands.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# like the makefile, recursively found cpp, cc, c.
file(GLOB_RECURSE SOURCES
    "src/*.c"
    "src/*.cpp"
    "src/*.cc"
)

# For each C file, get them compile as c.
foreach(SOURCE ${SOURCES})
    if(SOURCE MATCHES "\\.c$")
        set_source_files_properties(${SOURCE} PROPERTIES LANGUAGE CXX)
    endif()
endforeach()

# add executable, the final ./compiler.
add_executable(compiler ${SOURCES})
target_include_directories(compiler PUBLIC src)

# The tests make.
add_custom_target(test
    COMMAND python3 sp25-tests/test.py "$(shell git branch --show-current)" "${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running tests..."
)

# The format check.
add_custom_target(format
    COMMAND find src -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.def" \) -exec clang-format -i {} +
    COMMENT "Formatting source files..."
)