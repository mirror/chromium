function List(n) {
  if (n > 0)
    this.tail = new List(n - 1);
  else
    this.tail = null;
}

List.prototype.length = function () {
  var tail = this.tail;
  if (tail == null) return 0;
  else return 1 + tail.length();
};


print(new List(42).length());
print(new List(87).length());
