function Point(x, y) {
  this.x = x;
  this.y = y;
  this.getX = function () { return this.x; };
  this.getY = function () { return this.y; };
}


print(new Point(17, 18).getX());
print(new Point(17, 18)['getX']());
print(new Point(17, 18).getY());
print(new Point(17, 18)['getY']());


Point.prototype.foo = function () {
  return this.bar();
};

Point.prototype.bar = function () {
  return this.x;
};


print(new Point(42, 87).foo());

/*
function first() {
  return 1;
}

function second() {
  return this.first() + 1;
}

print(this.second());
*/