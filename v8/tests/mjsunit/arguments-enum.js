function countArguments() {
  var count = 0;
  for (var prop in arguments)
    count++;
  return count;
}

function setArgumentCount() {
  arguments[10] = 5;
  arguments.x = 4;
  var count = 0;
  for (var prop in arguments)
    count++;
  return count;
}

assertEquals(0, countArguments());
assertEquals(0, countArguments(1));
assertEquals(0, countArguments(1, 2));
assertEquals(0, countArguments(1, 2, 3, 4, 5));

assertEquals(0, setArgumentCount());
assertEquals(0, setArgumentCount(1));
assertEquals(0, setArgumentCount(1, 2));
assertEquals(0, setArgumentCount(1, 2, 3, 4, 5));
