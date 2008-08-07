// Should throw "RangeError: invalid array length"; V8 hits unimplemented()
// #6  0x0804db0a in V8_Fatal (file=0x80b8518 "/home/srdjan/v8/src/objects.cc", line=2173, format=0x80b8dd2 "unimplemented code")
//     at /home/srdjan/v8/src/checks.cc:12
// #7  0x08053e0d in v8::internal::JSObject::SetElementsLength (this=0xb7fdcb39, env=0x81073e8, len=0xb7f51c15)
//     at /home/srdjan/v8/src/objects.cc:2173
// #8  0x0806a11b in Builtin_ArrayConstruct (env=0x81073e8, __argc__=1, __argv__=0xbffff7a8) at /home/srdjan/v8/src/builtins.cc:134

try {
  var x = (new Array( Number(1e-7))).join()
} catch(e) {
  print("OK: exception thrown: " + e)
}
