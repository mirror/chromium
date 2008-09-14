var x = new Object();
x.foo = 'bar';
print(x.foo);
print(x['foo']);

x['bar'] = 87;
print(x['bar']);
print(x.bar);

x[5] = 42;
print(x[2 + 3]);

++x[5];
print(x[5]);

print(x[5]++);
print(x[5]);

var y = { foo: 'bar', 7: 42, 'baz': 87 };
print(y.foo);
print(y[7]);
print(y['baz']);
print(y.baz);
