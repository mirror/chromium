# Getting Started with libprotobuf-mutator in Chrome

*** note
**Warning:** libprotobuf-mutator is new to Chromium and does not (yet) have a 
track record of success. Writing fuzzers with libprotobuf-mutator will probably
require more effort than writing fuzzers with libFuzzer alone.

**Prerequisites:** Knowledge of [libFuzzer in Chromium] and basic understanding of 
[Protocol Buffers].
***

This document will walk you through:
* an overview of libprotobuf-mutator and how it's used.
* writing and building your first fuzzer using libprotobuf-mutator.

## Overview of libprotobuf-mutator
libprotobuf-mutator is a package that allows libFuzzer’s mutation engine to 
manipulate protobuffers. This allows libFuzzer's mutations to be more specific
to the format it is fuzzing. libprotobuf-mutator is particularly useful for 
fuzzing targets that accept Protocol Buffers as input. However, it is also 
useful for fuzzing targets that accept other kinds of input. For these formats,
you must write code that converts data in protobuf format to a format the
targeted code accepts. In the next two sections, we will discuss how to write 
and build fuzzers using libprotobuf-mutator. A working example of this, the
url_parse_proto_fuzzer can be found in 
`testing/libfuzzer/fuzzers/url_parse_proto_fuzzer.cc`,
`testing/libfuzzer/fuzzers/url.proto`. The build configuration for this fuzzer 
can be found in `testing/libfuzzer/fuzzers/BUILD.gn`.

## Write a libprotobuf-mutator fuzzer

Once you have in mind the code you want to fuzz and the format it accepts you 
are ready to start writing a libprotobuf-mutator fuzzer. Writing the fuzzer
will have three steps:

* Define the fuzzed format (not required for protobuf formats).
* Write the fuzzer function and conversion code (for non-protobuf formats).
* Define the GN target

### Define the Fuzzed Format
If you are fuzzing a target that accepts protocol buffers
Create a new .proto using `proto2` or `proto3` syntax and define a message that 
you want libFuzzer to mutate.

``` cpp
syntax = "proto2";

package my_fuzzer;

message MyFormat {
    // Define a format for libFuzzer to mutate here.
}
```

See `testing/libfuzzer/fuzzers/url.proto` for an example of this in practice.
That example has extensive comments on the URL syntax and how that influenced 
the definition of the Url message.

### Write the Fuzzer Function
Create a new .cc and define a `DEFINE_BINARY_PROTO_FUZZER` function:

```cpp
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"
// Assuming the .proto file is path/to/your/proto_file/my_format.proto.
#include "path/to/your/proto_file/my_format.pb.h"

DEFINE_BINARY_PROTO_FUZZER(const my_fuzzer::FuzzedFormat& fuzzed_format) {
  // Put your conversion code here (if needed) and then pass the result to 
  // your fuzzing code (or just pass "fuzzed_format", if your target accepts
  // protobufs).
}
```

This is very similar to the same step in writing a standard libFuzzer fuzzer.
The only real differences have to do with accepting protobufs and converting
them to the desired format. Conversion code can't really be explored in this
walkthough since it is format-specific. However, an good example of a fuzzer
function and conversion can be found in 
`testing/libfuzzer/fuzzers/url_parse_proto_fuzzer.cc` in practice. That example
thoroughly documents how it converts the Url protobuf message into a real URL
string.

### Define the GN Target
Define a fuzzer test target and include the your protobuf definiton and 
libprotobuf-mutator as dependencies.

```python
import("//testing/libfuzzer/fuzzer_test.gni")
import("//third_party/protobuf/proto_library.gni")

fuzzer_test("my_fuzzer") {
  sources = [ "my_fuzzer.cc" ]
  deps = [
    :my_format_proto
    "//third_party/libprotobuf-mutator"
    ...
  ]
}

proto_library("my_format_proto") {
  sources = [ "my_format.proto" ]
}
```

See `testing/libfuzzer/fuzzers/BUILD.gn` an example of this in practice.

### Wrap up Writing the Fuzzer
Once you have written a fuzzer with libprotobuf-mutator, building it, running
it, and adding it to clusterfuzz is almost the same as if the fuzzer were a
standard libFuzzer fuzzer (with minor exceptions, like your seed corpus must be
in protobuf format).

[libFuzzer in Chromium]: getting_started.md
[Protocol Buffers]: https://developers.google.com/protocol-buffers/docs/cpptutorial
