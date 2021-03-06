cmake_minimum_required (VERSION 3.6)
project (AtomicRingBuffer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT MSVC)
  add_compile_options(-Wall -Wextra -pedantic)
endif()

option(ENABLE_COVERAGE "enable_language measurement option add_compile_definitions coverage" OFF)

if (ENABLE_COVERAGE)
    include(CodeCoverage.cmake)
    append_coverage_compiler_flags()
endif()

find_program(CLANG_TIDY_EXECUTABLE clang-tidy)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()

enable_testing()

# Now simply link against gtest or gtest_main as needed. Eg
include_directories("test")
include_directories(".")

add_executable(AtomicRingBufferTest
    "AtomicRingBuffer/AtomicRingBuffer.cpp"
    "AtomicRingBuffer/StringCopyHelper.cpp"
    
    "test/Mocks.cpp"
    "test/AtomicRingBufferTest.cpp"
    "test/WraparoundTests.cpp"
    "test/StringCopyHelperTest.cpp"
    "test/ObjectRingBufferTest.cpp"
)
target_link_libraries(AtomicRingBufferTest gtest_main gmock)
add_test(NAME gtest_AtomicRingBufferTest_test COMMAND AtomicRingBufferTest)
target_compile_features(AtomicRingBufferTest PRIVATE cxx_std_14)

if (CLANG_TIDY_EXECUTABLE)
    set_target_properties(AtomicRingBufferTest PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE};-checks=*,-llvmlibc-callee-namespace,-modernize-use-trailing-return-type,-fuchsia-trailing-return,-llvmlibc-implementation-in-namespace,-llvmlibc-restrict-system-libc-headers")
endif()

if (MSVC)
    # Define for C2338 of MSVC.
    target_compile_definitions(AtomicRingBufferTest PRIVATE _ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()

if (ENABLE_COVERAGE)
setup_target_for_coverage_gcovr_html(
  NAME AtomicRingBufferTest-gcovr
  EXECUTABLE ctest
  DEPENDENCIES AtomicRingBufferTest
  EXCLUDE build test
)
endif()