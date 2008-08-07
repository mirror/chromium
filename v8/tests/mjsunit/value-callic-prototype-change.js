// Test that the inline caches correctly detect that constant
// functions on value prototypes change.

function testString() {
  function f(s, expected) {
    var result = s.toString();
    assertEquals(expected, result);
  };

  for (var i = 0; i < 10; i++) {
    var s = String.fromCharCode(i);
    f(s, s);
  }

  String.prototype.toString = function() { return "ostehaps"; };

  for (var i = 0; i < 10; i++) {
    var s = String.fromCharCode(i);
    f(s, "ostehaps");
  }
}

testString();


function testNumber() {
  Number.prototype.toString = function() { return 0; };

  function f(n, expected) {
    var result = n.toString();
    assertEquals(expected, result);
  };

  for (var i = 0; i < 10; i++) {
    f(i, 0);
  }

  Number.prototype.toString = function() { return 42; };

  for (var i = 0; i < 10; i++) {
    f(i, 42);
  }
}

testNumber();


function testBoolean() {
  Boolean.prototype.toString = function() { return 0; };

  function f(b, expected) {
    var result = b.toString();
    assertEquals(expected, result);
  };

  for (var i = 0; i < 10; i++) {
    f((i % 2 == 0), 0);
  }

  Boolean.prototype.toString = function() { return 42; };

  for (var i = 0; i < 10; i++) {
    f((i % 2 == 0), 42);
  }
}

testBoolean();
