/* Tests for Boolean */

function check(a, b) {
  if (a != b) {
    print("FAILED: the following two objects must be equal.");
    print(a);
    print(b);
  }
}

check(Boolean(undefined), false)
check(Boolean(null), false)
check(Boolean(false), false)
check(Boolean(true), true)
check(Boolean(0), false)
check(Boolean(1), true)
check(Boolean(check), true)
check(Boolean(new Object()), true)
check(new Boolean(false), false)
check(new Boolean(true), true)

check(true, !false);
check(false, !true);
check(true, !!true);
check(false, !!false);

check(true, true ? true : false);
check(false, false ? true : false);

check(false, true ? false : true);
check(true, false ? false : true);


check(true, true && true);
check(false, true && false);
check(false, false && true);
check(false, false && false);

// Regression.
var t = 42;
check(undefined, t.p);
check(undefined, t.p && true);
check(undefined, t.p && false);
check(undefined, t.p && (t.p == 0));
check(undefined, t.p && (t.p == null));
check(undefined, t.p && (t.p == t.p));

var o = new Object();
o.p = 'foo';
check('foo', o.p);
check('foo', o.p || true);
check('foo', o.p || false);
check('foo', o.p || (o.p == 0));
check('foo', o.p || (o.p == null));
check('foo', o.p || (o.p == o.p));

print("Done");

