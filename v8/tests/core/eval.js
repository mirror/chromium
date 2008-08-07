// global context access via eval()
for (var i = 0; i < 10; eval("i++"))
  print(eval("i"));

print("");

// local context access via eval()
function f() {
  for (var i = 0; i < 10; eval("i++"))
    print(eval("i"));
}

f();
