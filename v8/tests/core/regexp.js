function execute_re(re, s) {
  print(re.toString() + " matching " + s);
  var r = re.exec(s);
  print("result :");
  print("  re.lastIndex = " + re.lastIndex); 
  if (r) {
    print("  index = " + r.index);
    print("  input = " + r.input);
    print("  length = " + r.length);
    for (var i=0; i<r.length; i++) {
      print("    " + r[i]);
    }
  } else {
    print("  no match\n");
  }
}

execute_re(/(a|(z))(bc)/g, 'abcdef');
execute_re(/a|ab/, 'abc');
execute_re(/a[a-z]{2,4}/, "abcdefghi");
execute_re(/a[a-z]{2,4}?/, "abcdefghi");
execute_re(/(aa|aabaac|ba|b|c)*/, "aabaac");
execute_re(/(a*)b\1+/, "baaaac");
execute_re(/(?=(a+))/, "baaabac");
execute_re(/(?=(a+))a*b\1/, "baaabac");
// These two fail at the moment, returning empty strings for some of the
// captures where the correct answer was 'undefined'.
//execute_re(/(z)((a+)?(b+)?(c))*/, "zaacbbbcac");
//execute_re(/(a*)*/, "b");
// This fails at the moment.  To quote someone on the ActionScript bug database:
// "problem is that ECMA say backreference into a negative lookahead should
//  always return null and pcre doesn't work that way. Need to hack pcre to do
//  the right thing."
//execute_re(/(.*?)a(?!(a+)b\2c)\2(.*)/, "baaabaac");

var re = new RegExp;
print(re);                  // /(?:)/

// test regexp escaping
var re = /\x51G/;         
print(re);                  // /\x51G/
print(re.exec("QG"));       // QG
print(re.exec("\x51G"));    // QG

var re = /\x5G/;          
print(re);                  // /\x5G/ 
print(re.exec("x5G"));      // x5G
print(re.exec("\x05G"));    // null

var re = /\u0051G/;     
print(re);                  // /\u0051G/
print(re.exec("QG"));       // QG
print(re.exec("\u0051G"));  // QG
print(re.exec("\u051G"));   // null

var re = /\u051G/;       
print(re);                  // /\051G/
print(re.exec("\u0051G"));  // null
print(re.exec("u051G"));    // u051G

var re = /\uXY/;        
print(re);                  // /\uXY/
print(re.exec("uXY"));      // uXY
