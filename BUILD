cc_library(
    name = "AtomicRingBuffer",
    srcs = glob(["AtomicRingBuffer/*.cpp"]),
    hdrs = glob(["AtomicRingBuffer/*.h"]),
    copts = select({
        "@platforms//os:windows": ["/std:c++14", "/permissive-"],
        "//conditions:default": ["-std=c++14", "-Wall", "-Wextra", "-Wpedantic"],
    }),
)

cc_test(
    name = "AtomicRingBuffer_Test",
    size = "small",
    srcs = glob(["test/*.cpp", "test/*.h"]),
    deps = [
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "AtomicRingBuffer",
    ],
)

# TODO
# MSVC

# C++14 standard without compiler extensions
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Code Coverage

# integate clang-tidy

# compiler options non-msvc
# add_compile_options(-Wall -Wextra -pedantic)