// ecma/String/15.5.3.1-2.js
//
// Recursiveley throwing too many exceptions
// 
// #6  0x0804db0a in V8_Fatal (file=0x80bb187 "/home/srdjan/v8/src/heap.cc", line=1689, format=0x80bae2e "CHECK(%s) failed")
//     at /home/srdjan/v8/src/checks.cc:12
// #7  0x080b05dd in v8::internal::HandleScopeStack::Push () at /home/srdjan/v8/src/heap.cc:1689
// #8  0x08061991 in v8::internal::Heap::PushHandleBlock () at /home/srdjan/v8/src/heap.cc:1711
// #9  0x080adde5 in v8::internal::HandleScope::Push () at handles-inl.h:21
// #10 0x08091296 in v8::internal::JSEnv::CreateTypeError (this=0x81083e8, type=0x80beef7 "undefined_method")
//     at /home/srdjan/v8/src/jsenv.cc:489
// #11 0x08077dd0 in v8::internal::ICC::Lookup (env=0x81083e8, receiver=0xb7fdd345, name=0xb7f266a5) at /home/srdjan/v8/src/ic.cc:141
// #12 0x08077fcc in v8::internal::ICC::Miss (this=0xb7f507c9, env=0x81083e8, object=0xb7fdd345, name=0xb7f266a5)
//     at /home/srdjan/v8/src/ic.cc:107
// #13 0x0807ad51 in v8::internal::ICC::Initialize (this=0xb7f507c9, env=0x81083e8, object=0xb7f4fcd1, name=0xb7f266a5)
//     at /home/srdjan/v8/src/ic.cc:75
// #14 0x0807af51 in ICC_Initialize (env=0x81083e8, args={length_ = 1, arguments_ = 0xbfffcf0c}) at /home/srdjan/v8/src/ic.cc:536

function TestCase( e, a ) {
  print(e)
}

new TestCase(String.prototype,
             eval("String.prototype=null;String.prototype") );
