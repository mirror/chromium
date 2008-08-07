print("Hello," + " World!");
print("1 + 2 = " + (1 + 2));
print((3 + 4) + " = 3 + 4");

print(String(null))
print(String(false))
print(String(true))
print(String("fisk"))

print("".length);
print("Hello".length);
print("foo bar hest".length);

print(parseInt("101", 16));
print(parseInt("0", 2));
print(parseInt("0"));
print(parseInt("42"));
print(parseInt("1001", 2));


print(String.fromCharCode().length);
print(String.fromCharCode(0x41, 0x62, 0x65).length);
print(String.fromCharCode(0x41, 0x62, 0x65));

//test string functions
print("Hello world".match(/Hello/)[0]); // Hello
print("Hello world".match(/hello/));    // null

print("Hello world".search(/world/));   // 6
print("Hello world".search(/World/));   // -1

print("Hello world".slice(0, 5));    // Hello
print("Hello world".slice(0));       // Hello world
print("Hello world".slice(-5));      // world
print("Hello world".slice(-5, -3));  // wo

print("Hello world".substring(0, 5)); // Hello
print("Hello world".substring(0));    // Hello world
print("Hello world".substring(-5));   // Hello world
print("Hello world".substring(-5, -3)); // ""
print("Hello world".substring(NaN));  // Hello world

print("Hello world".substr(100, 5)); // ""
print("Hello world".substr(-100, 5)); // Hello
print("Hello world".substr(0, 0)); // ""
print("Hello world".substr(0, -5)); // ""
print("Hello world".substr(-5, 10)); // world

function replace_func() {
  return("FOO" + arguments[0] + "BAR");
}
 
var s = "abc1234defg1234hello";
print(s.replace(/(\d)+/g, replace_func));
print(s.replace("1234", "5678"));
print(s.replace(/\d+/, "5678"));
print(s.replace(/\d+/g, "5678"));
print(s.replace(/(\d+)/, "$&"));
print(s.replace(/(\d+)/, "$`"));
print(s.replace(/(\d+)/, "$'"));
print("$1,$2".replace(/(\$(\d))/g, "$$1-$1$2"));
print(s.replace('abc', 89));

// test localeCompare without arguments
print(''.localeCompare());  // 0
print('a'.localeCompare());  // 0
print(''.localeCompare(undefined));  
