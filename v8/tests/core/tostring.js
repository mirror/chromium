function Point(x,y) {
  this.x = x;
  this.y = y;
}


Point.prototype.toString = function() {
  gc();
  return '(' + this.x + ',' + this.y + ')';  
};


print(new Point(16, 17));
print(new Point(5, 12));
