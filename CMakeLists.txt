cmake_minimum_required(VERSION 3.16)
project(Sif
    LANGUAGES CXX
    HOMEPAGE_URL https://github.com/atjhc/sif
)

# Enable all warnings, and treat warnings as errors.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Allow selection of standard library.
option(USE_LIBCXX "Use libc++ instead of GNU libstdc++" ON)
if(USE_LIBCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
endif()

# Set build directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Define build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "Fuzz")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Using standard library: ${CMAKE_CXX_FLAGS}")

# On macOS systems, Xcode's version of clang doesn't include fuzzing support, so try homebrew.
if(CMAKE_BUILD_TYPE STREQUAL "Fuzz")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    find_program(HOMEBREW_LLVM_CLANG NAMES clang PATHS "/opt/homebrew/opt/llvm/bin" NO_DEFAULT_PATH)
    if(HOMEBREW_LLVM_CLANG)
        message(STATUS "Using Homebrew LLVM Clang: ${HOMEBREW_LLVM_CLANG}")
        set(CMAKE_C_COMPILER "${HOMEBREW_LLVM_CLANG}")
        set(CMAKE_CXX_COMPILER "${HOMEBREW_LLVM_CLANG}++")
    endif()
endif()

# Collect source files
file(GLOB_RECURSE SIF_SOURCES "src/*.cc")

# Exclude tools and tests from the library build
list(FILTER SIF_SOURCES EXCLUDE REGEX "src/tools/.*")
list(FILTER SIF_SOURCES EXCLUDE REGEX "src/tests/.*")
list(FILTER SIF_SOURCES EXCLUDE REGEX "src/extern/.*")

# Create a static library
add_library(sif STATIC ${SIF_SOURCES})
target_include_directories(sif PRIVATE "src")
target_include_directories(sif PUBLIC "include")

target_compile_features(sif PUBLIC cxx_std_20)

# Define executable for the main tool
add_executable(sif_tool "src/tools/sif.cc")
target_link_libraries(sif_tool PRIVATE sif)
target_include_directories(sif_tool PRIVATE "src")
target_include_directories(sif_tool PUBLIC "include")

# Define executable for fuzzing
if(CMAKE_BUILD_TYPE STREQUAL "Fuzz")
    add_executable(sif_fuzz src/tools/sif.cc)
    target_link_libraries(sif_fuzz PRIVATE sif)
    target_link_options(sif_fuzz PRIVATE -fsanitize=fuzzer,address)
    target_include_directories(sif_fuzz PRIVATE src)
    target_compile_options(sif_fuzz PRIVATE -g -fsanitize=fuzzer,address -DFUZZER -Wno-unused-function -Wno-unused-variable)

    set(FUZZROOT "${CMAKE_BINARY_DIR}/fuzz")
    add_custom_command(TARGET sif_fuzz POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FUZZROOT}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/tests/resources/transcripts ${FUZZROOT}/corpus
        COMMAND ASAN_OPTIONS=detect_container_overflow=0 LLVMFuzzer_DATA_PATH=${FUZZROOT} ${CMAKE_BINARY_DIR}/sif_fuzz -artifact_prefix=${FUZZROOT}/ ${FUZZROOT}/corpus
    )
endif()

# Enable testing
include(CTest)
enable_testing()

# Find test files
file(GLOB_RECURSE TEST_SOURCES "src/tests/*.cc")
add_executable(sif_tests ${TEST_SOURCES} src/tools/tests.cc)
target_link_libraries(sif_tests PRIVATE sif)
target_include_directories(sif_tests PRIVATE "src")
file(COPY src/tests/resources DESTINATION ${CMAKE_BINARY_DIR})
file(COPY src/tests/repl_tests.sh DESTINATION ${CMAKE_BINARY_DIR})

# Define a test
add_test(NAME SifUnitTests COMMAND sif_tests resources)
add_test(NAME SifREPLTests COMMAND repl_tests.sh "${CMAKE_BINARY_DIR}/sif_tool")

# Set debug macro for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    target_compile_definitions(sif PRIVATE DEBUG=1)
    target_compile_definitions(sif_tool PRIVATE DEBUG=1)
    target_compile_definitions(sif_tests PRIVATE DEBUG=1)
endif()
