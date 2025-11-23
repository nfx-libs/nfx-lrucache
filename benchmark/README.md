# Benchmarks

---

## Test Environment

### Hardware Configuration

| Component                | Specification                                                 |
| ------------------------ | ------------------------------------------------------------- |
| **Computer Model**       | Lenovo ThinkPad P15v Gen 3                                    |
| **CPU**                  | 12th Gen Intel Core i7-12800H (20 logical, 14 physical cores) |
| **Base Clock**           | 2.80 GHz                                                      |
| **Turbo Clock**          | 4.80 GHz                                                      |
| **L1 Data Cache**        | 48 KiB (×6 P-cores) + 32 KiB (×8 E-cores)                     |
| **L1 Instruction Cache** | 32 KiB (×6 P-cores) + 64 KiB (×2 E-core clusters)             |
| **L2 Unified Cache**     | 1.25 MiB (×6 P-cores) + 2 MiB (×2 E-core clusters)            |
| **L3 Unified Cache**     | 24 MiB (×1 shared)                                            |
| **RAM**                  | DDR4-3200 (32GB)                                              |
| **GPU**                  | NVIDIA RTX A2000 4GB GDDR6                                    |

### Software Configuration

| Platform    | OS         | Benchmark Framework     | C++ Compiler              | nfx-lrucache Version |
| ----------- | ---------- | ----------------------- | ------------------------- | -------------------- |
| **Linux**   | LMDE 7     | Google Benchmark v1.9.4 | GCC 14.2.0-x64            | v0.1.0               |
| **Windows** | Windows 10 | Google Benchmark v1.9.4 | MinGW GCC 14.2.0-x64      | v0.1.0               |
| **Windows** | Windows 10 | Google Benchmark v1.9.4 | Clang-GNU-CLI 19.1.5-x64  | v0.1.0               |
| **Windows** | Windows 10 | Google Benchmark v1.9.4 | Clang-MSVC-CLI 19.1.5-x64 | v0.1.0               |
| **Windows** | Windows 10 | Google Benchmark v1.9.4 | MSVC 19.44.35217.0-x64    | v0.1.0               |

---

# Performance Results

## LRU Cache Operations

| Operation                | Linux GCC   | Linux Clang | Windows MinGW GCC | Windows Clang-GNU-CLI | Windows Clang-MSVC-CLI | Windows MSVC |
| ------------------------ | ----------- | ----------- | ----------------- | --------------------- | ---------------------- | ------------ |
| Construction_Default     | 13.4 ns     | **12.7 ns** | 65.6 ns           | 153 ns                | 105 ns                 | 85.1 ns      |
| Construction_WithOptions | 108 ns      | **102 ns**  | 431 ns            | 544 ns                | 375 ns                 | 476 ns       |
| Get_NewEntry             | **74.6 ns** | 75.0 ns     | 253 ns            | 461 ns                | 412 ns                 | 370 ns       |
| Get_ExistingEntry        | 33.3 ns     | **31.9 ns** | 141 ns            | 84.4 ns               | 58.6 ns                | 47.5 ns      |
| Get_WithConfig           | **86.3 ns** | 88.4 ns     | 271 ns            | 552 ns                | 461 ns                 | 382 ns       |
| Find_Hit                 | 34.8 ns     | **31.8 ns** | 137 ns            | 91.7 ns               | 64.1 ns                | 48.7 ns      |
| Find_Miss                | **4.96 ns** | 5.37 ns     | 18.7 ns           | 28.8 ns               | 19.2 ns                | 14.6 ns      |
| Remove                   | 5.48 µs     | **5.39 µs** | 23.1 µs           | 23.6 µs               | 16.1 µs                | 15.1 µs      |
| Clear/10                 | 244 ns      | **241 ns**  | 634 ns            | 878 ns                | 614 ns                 | 497 ns       |
| Clear/100                | 808 ns      | **798 ns**  | 2.22 µs           | 2.98 µs               | 2.10 µs                | 1.96 µs      |
| Clear/1000               | 5.41 µs     | **5.37 µs** | 16.6 µs           | 21.9 µs               | 16.0 µs                | 15.4 µs      |
| Clear/10000              | 106 µs      | **102 µs**  | 133 µs            | 218 µs                | 168 µs                 | 164 µs       |
| Size                     | 2.67 ns     | **2.62 ns** | 10.8 ns           | 18.6 ns               | 14.6 ns                | 13.0 ns      |
| IsEmpty                  | **2.67 ns** | 2.85 ns     | 13.7 ns           | 19.3 ns               | 15.4 ns                | 14.0 ns      |
| LRU_Eviction             | **774 ns**  | 846 ns      | 1.69 µs           | 2.47 µs               | 2.02 µs                | 2.02 µs      |
| LRU_Access_Pattern       | 4.42 µs     | **4.32 µs** | 9.26 µs           | 10.4 µs               | 8.68 µs                | 8.12 µs      |
| CleanupExpired/10        | 387 ns      | **370 ns**  | 784 ns            | 977 ns                | 838 ns                 | 738 ns       |
| CleanupExpired/100       | 2.38 µs     | **2.23 µs** | 5.00 µs           | 5.21 µs               | 4.60 µs                | 4.06 µs      |
| CleanupExpired/1000      | 22.4 µs     | **21.2 µs** | 47.1 µs           | 48.0 µs               | 44.2 µs                | 38.9 µs      |
| ComplexValue_Vector      | 916 ns      | **898 ns**  | 1.59 µs           | 1.96 µs               | 1.91 µs                | 1.72 µs      |
| ComplexValue_String      | **513 ns**  | 538 ns      | 913 ns            | 706 ns                | 656 ns                 | 585 ns       |
| Scenario_DatabaseCache   | 40.0 ns     | **38.4 ns** | 85.5 ns           | 61.2 ns               | 56.4 ns                | 54.8 ns      |
| Scenario_WebCache        | **34.4 ns** | 35.0 ns     | 84.5 ns           | 66.7 ns               | 63.3 ns                | 58.5 ns      |

---

_Benchmarks executed on November 23, 2025_
