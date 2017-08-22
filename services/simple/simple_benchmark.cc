// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "simple_api.h"

#include <cstdio>
#include <memory>
#include <string>

// Information related to each shared library loaded by this benchmark.
struct LibraryInfo {
  const char* name;  // Library name, without the "libsimple_api_" prefix.
  int load_count;    // Number of times this library should be loaded.
};

static const LibraryInfo kLibraries[] = {
    {"direct", 100},        {"std_function", 100}, {"callback", 100},
    {"lambda", 100},        {"bind_once", 10},     {"bind_repeating", 10},
    {"post_msgloop", 10},   {"post_sequence", 10}, {"post_single_thread", 10},
    {"thread_condvar", 10}, {"thread_event", 10},  {"mojo_async", 10},
    {"mojo_sync", 10},
};

// Small convenience class to collect time differences every time
// its delta() method is called.
class TicksCounter {
 public:
  TicksCounter() : ticks_(base::TimeTicks::Now()) {}

  base::TimeDelta delta() {
    base::TimeTicks now = base::TimeTicks::Now();
    base::TimeDelta result = now - ticks_;
    ticks_ = now;
    return result;
  }

 private:
  base::TimeTicks ticks_;
};

// Convenience class to hold a scoped shared library handle and its createXXX()
// function pointers. The library is unloaded on scope exit.
class ScopedSimpleLibrary {
 public:
  ScopedSimpleLibrary(const char* name) {
    std::string libname("simple_api_");
    libname += name;
    libname = base::GetNativeLibraryName(libname);

    base::FilePath path;
    CHECK(base::PathService::Get(base::DIR_EXE, &path))
        << "Can't find current program directory!";

    path = path.AppendASCII(libname);

    base::NativeLibraryLoadError error;
    module_ = base::LoadNativeLibrary(path, &error);
    CHECK(module_) << "Could not load library: " << path.AsUTF8Unsafe();

    createMath_ = reinterpret_cast<simple::Math* (*)()>(
        base::GetFunctionPointerFromNativeLibrary(module_, "createMath"));
    CHECK(createMath_);

    createEcho_ = reinterpret_cast<simple::Echo* (*)()>(
        base::GetFunctionPointerFromNativeLibrary(module_, "createEcho"));
    CHECK(createEcho_);

    createAbstract_ = reinterpret_cast<const char* (*)()>(
        base::GetFunctionPointerFromNativeLibrary(module_, "createAbstract"));
    CHECK(createAbstract_);
  }

  // Explicitely unload the library. It's up to the caller to ensure that
  // there are no lingering references / pointers to the library's code and
  // data anywhere else.
  void Unload() {
    if (module_) {
      base::UnloadNativeLibrary(module_);
      module_ = nullptr;
    }
  }

  simple::Math* createMath() const { return createMath_(); }
  simple::Echo* createEcho() const { return createEcho_(); }
  const char* abstract() const { return createAbstract_(); }

  ~ScopedSimpleLibrary() { Unload(); }

 private:
  base::NativeLibrary module_ = nullptr;
  simple::Math* (*createMath_)() = nullptr;
  simple::Echo* (*createEcho_)() = nullptr;
  const char* (*createAbstract_)() = nullptr;
};

void BenchmarkLibrary(const LibraryInfo& lib_info) {
  const int kLibraryLoadCount = lib_info.load_count;
  const int kMathCallCount = 100000;
  const int kEchoCallCount = 100000;

  base::TimeDelta library_load_delta;
  base::TimeDelta library_unload_delta;
  base::TimeDelta math_interface_delta;
  base::TimeDelta math_call_delta;
  base::TimeDelta echo_interface_delta;
  base::TimeDelta echo_call_delta;

  for (int n_lib = 0; n_lib < kLibraryLoadCount; ++n_lib) {
    TicksCounter counter;
    ScopedSimpleLibrary lib(lib_info.name);
    library_load_delta += counter.delta();

    std::unique_ptr<simple::Math> math(lib.createMath());
    math_interface_delta += counter.delta();

    for (int n_math = 0; n_math < kMathCallCount; ++n_math) {
      int32_t x = 10;
      int32_t y = 20;
      (void)math->Add(x, y);
    }

    math_call_delta += counter.delta();
    math.reset();  // Destroy interface now.
    math_interface_delta += counter.delta();

    std::unique_ptr<simple::Echo> echo(lib.createEcho());
    echo_interface_delta += counter.delta();

    std::string input("Hello foolish world. This is a long long string");
    std::string output;
    for (int n_echo = 0; n_echo < kEchoCallCount; ++n_echo) {
      output = echo->Ping(input);
    }
    echo_call_delta += counter.delta();
    echo.reset();  // Destroy interface now.
    echo_interface_delta += counter.delta();

    lib.Unload();
    library_unload_delta += counter.delta();
  }

  double library_load_time_us =
      library_load_delta.InNanoseconds() / (1000. * kLibraryLoadCount);
  double library_unload_time_us =
      library_unload_delta.InNanoseconds() / (1000. * kLibraryLoadCount);
  double math_interface_time_ns =
      math_interface_delta.InNanoseconds() / (1. * kLibraryLoadCount);
  double echo_interface_time_ns =
      echo_interface_delta.InNanoseconds() / (1. * kLibraryLoadCount);
  double math_call_time_ns = math_call_delta.InNanoseconds() /
                             (1. * kLibraryLoadCount * kMathCallCount);
  double echo_call_time_ns = echo_call_delta.InNanoseconds() /
                             (1. * kLibraryLoadCount * kEchoCallCount);

  printf("%-22s:  %14.1f | %14.1f | %14.1f | %14.1f | %14.1f | %14.1f\n",
         lib_info.name, math_call_time_ns, echo_call_time_ns,
         math_interface_time_ns, echo_interface_time_ns, library_load_time_us,
         library_unload_time_us);
}

int main(int argc, char** argv) {
  if (argc > 1) {
    if (::strcmp(argv[1], "--help") != 0) {
      fprintf(stderr, "This program doesn't take parameters. See --help\n");
      return 1;
    }
    printf(
        "A simple benchmark used to measure various ways to implement trivial\n"
        "interfaces for loadable components implemented in shared "
        "libraries.\n\n"

        "The first interface (math) is used to add two 32-bit integers and\n"
        "return their sum. The second interface (echo) receives a std::string\n"
        "and sends it back as the result\n\n"

        "The benchmark tries to measure the following steps:\n\n"

        "  - math call: Time to call a single Add() method from the Math\n"
        "               interface and get its result.\n\n"

        "  - echo call: Time to call a single Ping() method from the Echo\n"
        "               interface and get its result.\n\n"

        "  - math int.: Time to create and destroy a single Math interface\n"
        "               implementation from the shared library.\n\n"

        "  - echo int.: Time to create and destroy a single Echo interface\n"
        "               implementation from the shared library.\n\n"

        "  - load: Shared library load time, which includes running all "
        "static\n"
        "          C++ initializers for the corresponding code.\n\n"

        "  - unload: Shared library unload time, which includes running all\n"
        "            C++ finalizers for the corresponding code.\n\n"

        "The following interface implementations are tested:\n\n");
    for (const auto lib : kLibraries) {
      ScopedSimpleLibrary scoped(lib.name);
      printf("  - %-15s : %s\n", lib.name, scoped.abstract());
    }
    printf("\n");
    return 0;
  }

  printf("%-22s:  %14s | %14s | %14s | %14s | %14s | %14s\n", "times in us",
         "math call (ns)", "echo call (ns)", "math int (ns)", "echo int (ns)",
         "load (us)", "unload (us)");
  for (const auto lib : kLibraries) {
    BenchmarkLibrary(lib);
  }
  return 0;
}
