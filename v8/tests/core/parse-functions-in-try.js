// 'Unexpected token function'.
// If the try-catch is removed, the code is run without problem.
// (Note: This is not 'try'-related at all, it applies to any compound
// statement. The issue is really that the ECMA-262 spec DOES NOT
// allow function declarations inside statements at all, but that we
// nevertheless need to allow them.)

if (true) {
  function f(){} function g(){}
}

try {
  function f(){} function g(){}
} catch (e) {
}
