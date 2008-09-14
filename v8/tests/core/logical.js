// According to ECMA-262 section 11.11, page 58, the binary logical
// operators must yield the result of one of the two expressions
// before any ToBoolean() conversions. This means that the value
// produced by a && or || operator is not necessarily a boolean.

var undef;

print(1 || 0);  // 1
print(0 || 2);  // 2
print(0 || 'foo');  // 'foo'
print(undef || undef);  // undefined
print('foo' || 'bar');  // 'foo'
print(undef || 'bar');  // 'bar'
print(undef || 'bar' || 'baz');  // 'bar'
print(undef || undef || 'baz');  // 'baz'

print(1 && 0);  // 0
print(0 && 2);  // 0
print(0 && 'foo');  // 0
print(undef && undef);  // undefined
print('foo' && 'bar');  // 'bar'
print(undef && 'bar');  // undefined
print('foo' && 'bar' && 'baz');  // 'baz'
print('foo' && undef && 'baz');  // undefined

print(undef && undef || 0);  // 0
print(undef && 0 || 'bar');  // 'bar'
