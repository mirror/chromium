// The global object shouldn't not have any properties that start with
// a dot. They should be local to the script invocation (on the
// stack).

var ok = true;
for (var p in this) {
  if (p.charAt(0) == '.')
    ok = false;
}

if (ok) print("SUCCESS"); else print("FAILED");
