// This is a reduced version of the mozilla test
// js1_5/Regress/regress-146596.js.
//
// This failed with a ReferenceError because result could not be found.

function f() {
  var result = {};

  try {
    throw 42;
  } catch (e) {
    with ({ x: 12 }) {
      result.property = 42;
    }
  }
}

f();

print("Success.");


