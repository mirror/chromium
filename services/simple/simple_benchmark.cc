// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/time/time.h"
#include "simple_api.h"

#include <cstdio>
#include <memory>
#include <string>

#include <dlfcn.h>
#include <string.h>

// Information related to each shared library loaded by this benchmark.
struct LibraryInfo {
  const char* name;      // Library name, without the "libsimple_api_" prefix.
  const char* abstract;  // Small one-line description of the library.
  int load_count;        // Number of times this library should be loaded.
};

static const LibraryInfo kLibraries[] = {
    {"direct", "direct implementation of each interface", 100},
    {"std_function", "std::function<> based implementation", 100},
    {"callback", "base::Callback<> based implementation", 100},
    {"mojo_async", "uses Mojo asynchronous C++ bindings", 10},
    {"mojo_sync", "uses Mojo synchronous C++ bindings", 10},
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
    std::string libname("libsimple_api_");
    libname += name;
    libname += ".so";

    module_ = ::dlopen(libname.c_str(), RTLD_NOW);
    CHECK(module_);

    createMath_ =
        reinterpret_cast<simple::Math* (*)()>(::dlsym(module_, "createMath"));
    CHECK(createMath_);

    createEcho_ =
        reinterpret_cast<simple::Echo* (*)()>(::dlsym(module_, "createEcho"));
    CHECK(createEcho_);
  }

  // Explicitely unload the library. It's up to the caller to ensure that
  // there are no lingering references / pointers to the library's code and
  // data anywhere else.
  void Unload() {
    if (module_) {
      ::dlclose(module_);
      module_ = nullptr;
    }
  }

  simple::Math* createMath() const { return createMath_(); }
  simple::Echo* createEcho() const { return createEcho_(); }

  ~ScopedSimpleLibrary() { Unload(); }

 private:
  void* module_;
  simple::Math* (*createMath_)() = nullptr;
  simple::Echo* (*createEcho_)() = nullptr;
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

  printf("%-16s:  %14.1f | %14.1f | %14.1f | %14.1f | %14.1f | %14.1f\n",
         lib_info.name, library_load_time_us, math_interface_time_ns,
         math_call_time_ns, echo_interface_time_ns, echo_call_time_ns,
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

        "  - load: Shared library load time, which includes running all "
        "static\n"
        "          C++ initializers for the corresponding code.\n\n"

        "  - unload: Shared library unload time, which includes running all\n"
        "            C++ finalizers for the corresponding code.\n\n"

        "  - math int.: Time to create and destroy a single Math interface\n"
        "               implementation from the shared library.\n\n"

        "  - math call: Time to call a single Add() method from the Math\n"
        "               interface and get its result.\n\n"

        "  - echo int.: Time to create and destroy a single Echo interface\n"
        "               implementation from the shared library.\n\n"

        "  - echo call: Time to call a single Ping() method from the Echo\n"
        "               interface and get its result.\n\n"

        "The following interface implementations are tested:\n\n");
    for (const auto lib : kLibraries) {
      printf("  - %-15s : %s\n", lib.name, lib.abstract);
    }
    printf("\n");
    return 0;
  }

  printf("%-16s:  %14s | %14s | %14s | %14s | %14s | %14s\n", "times in us",
         "load (us)", "math int (ns)", "math call (ns)", "echo int (ns)",
         "echo call (ns)", "unload (us)");
  for (const auto lib : kLibraries) {
    BenchmarkLibrary(lib);
  }
  return 0;
}
