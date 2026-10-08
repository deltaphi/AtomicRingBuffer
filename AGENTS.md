# Repository guidance

## Build and verify

- C++14 is required, with compiler extensions disabled. CMake builds one executable, `AtomicRingBufferTest`, directly from library and test sources; there is no separate library target. Add new `.cpp` files explicitly to `CMakeLists.txt`.
- Linux/devcontainer CI sequence, run from the repository root:
  ```sh
  cmake --preset Linux
  cmake --build --preset build-Linux
  ./build/AtomicRingBufferTest
  ```
- Windows uses `cmake --preset Windows`, `cmake --build --preset build-Windows`, then `build\Release\AtomicRingBufferTest.exe`. The preset specifically requires the `Visual Studio 18 2026` generator.
- Configuration downloads GoogleTest/GoogleMock v1.13.0 via Git (`CMakeLists.txt.in`); a fresh build needs Git and network access. Downloaded sources live under `build/`.
- CTest registers the entire executable as one test. For focused tests, use GoogleTest directly, e.g. `./build/AtomicRingBufferTest --gtest_filter='BufferedAtomicBufferFixture.FullCycle'`; discover names with `--gtest_list_tests`.
- If `clang-tidy` is found at configure time, CMake automatically runs it during compilation with the checks configured in `CMakeLists.txt`.
- Format changed C++ files with `clang-format -i <files>` using the root `.clang-format`. `apply-clang.sh` hardcodes `clang-format.exe` and reformats both source directories, so it is Windows-oriented.

## Buffer invariants

- The byte buffer uses caller-owned storage and single-reader/single-writer semantics. Multiple readers or writers require external synchronization within their respective groups; atomic indices do not make this an MPMC queue.
- Preserve the ordered `allocate` → `publish` and `peek` → `consume` protocol. Returned ranges never cross the physical buffer boundary; partial requests and commits are intentional, and out-of-order commits are rejected.
- Indices wrap at **twice** the buffer capacity to distinguish full from empty. Preserve the circular `readIdx <= writeIdx <= allocateIdx` invariant documented in `AtomicRingBuffer/AtomicRingBuffer.h`; do not simplify indices to modulo capacity.
- `ObjectRingBuffer<T, ...>` is a header-only adapter over the byte buffer with internally owned aligned storage. It converts byte counts to element counts but does not construct or destroy `T` objects.
