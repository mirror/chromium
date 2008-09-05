// This test case segfaults in generated code. See
// buganizer issue #1066899.
function Crash() {
  for (var key in [0]) {
    try { } finally { continue; }
  }
}

Crash();

