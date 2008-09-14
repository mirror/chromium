function f1(s) {
  var y1 = "y1";

  function f2(s) {
    var y2 = "y2";

    function f3(s) {
      y3 = "y3";
      eval(s);
      print("y3 = " + y3);
    }

    f3(s);
    print("y2 = " + y2);
  }

  f2(s);
  print("y1 = " + y1);
  print();
}

var y0 = "y0";
f1("y0 = \"foo\";");
f1("y1 = \"foo\";");
f1("y2 = \"foo\";");
f1("y3 = \"foo\";");
print("y0 = " + y0);
print();

var y0 = "y0";
f1("var y0 = \"foo\";");
f1("var y1 = \"foo\";");
f1("var y2 = \"foo\";");
f1("var y3 = \"foo\";");
print("y0 = " + y0);
print();
