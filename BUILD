cc_library(
    name = "AtomicRingBuffer",
    srcs = glob(["AtomicRingBuffer/*.cpp"]),
    hdrs = glob(["AtomicRingBuffer/*.h"]),
    copts = select({
        "@platforms//os:windows": ["/std:c++14"],
        "//conditions:default": ["-Wall", "-Wextra", "-Wpedantic"],
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
