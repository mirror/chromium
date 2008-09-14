/*

This fuzzer creates random, possibly invalid, JavaScript function bodies.  

It does this using recursion, thinking loosely in terms of "statements", "expressions", "lvalues", "literals", etc.

Once it creates a function body, it does the following things with it:
* Splits it in half and tries to compile each half, mostly to find bugs in the compiler's error-handling.
* Compiles it
* Decompiles it (using both toString and uneval)
* Checks two things about each decompilation: 
  * Does it compile? 
  * The "round-trip" test: when passed through the compiler and decomiler a second time, 
    is the decompilation exactly the same? see below**
* Executes it
* If executing returned a generator, loops through the generator.


**This "round-trip change test" is intended to find bugs where the first
decompilation was incorrect.  It is effective at finding bugs, but it also hits
false positives -- situations where a round-trip changes the decompilation, even
though the first decompilation was not really incorrect, just not fully
optimized or not canonical.  Brendan has been nice about fixing these
"round-trip change" bugs even when they're not really bugs, since he sees value
in this type of fuzzing.


Bugs in SpiderMonkey found with this fuzzer are tracked by https://bugzilla.mozilla.org/show_bug.cgi?id=349611.



Some things to test this with:

JS Shell                        make -f Makefile.ref
JS Shell with extra GCing       make XCFLAGS=-DWAY_TOO_MUCH_GC -f Makefile.ref
JS Shell without assertions     make BUILD_OPT=1 OPTIMIZER=-Os -f Makefile.ref
Firefox

Valgrind?
Purify?



Todo:
  Keep stats on exclusions (80% executed, 50% decompiled) and lengths?



Useful references:
  http://www.codehouse.com/javascript/precedence/
  http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference
  http://developer.mozilla.org/en/docs/New_in_JavaScript_1.6
  http://developer.mozilla.org/en/docs/New_in_JavaScript_1.7

*/

if (typeof randomSeed == "undefined") {
  var randomSeed = 6;
}

if (typeof runCount == "undefined") {
  var runCount = 1000;
}

// Basic compatibility with Safari/WebKit and Mozilla's command-line jsshell.
var haveUneval = (typeof uneval == "function") ? true : false;
var jsshell = (typeof window == "undefined");
if (jsshell) {
  dump = print;
  dumpln = print;

  alert = function(s) { dumpln(""); dumpln("Alert!? Alert is mapped to quit! Alerted text follows."); dumpln(s); quit(); }
  dumplnAndAlert = function(s) { dumpln(s); quit(); }

  version(170); // make "yield" and "let" work.
}
if (!jsshell) {
  if (typeof dump != "function") dump = function() { };
  dumpln = function(s) { dump(s + "\n"); }

  gc = function(){}; // :(
  dumplnAndAlert = function(s) { 
    dumpln(s);
    var p = document.createElement("pre");
    p.appendChild(document.createTextNode(s));
    document.body.appendChild(p);
  }
}


// Be able to restore sanity...
var realEval = eval;
var realFunction = Function;
var realGC = gc;









// Given a seed, returns a function that generates random integers.
// Based on the Central Randomizer 1.3 (C) 1997 by Paul Houle (paul@honeylocust.com)
// See: http://www.honeylocust.com/javascript/randomizer.html
function randomizer(seed)
{
  // Returns a somewhat random real number between 0 (inclusive) and 1 (not inclusive), like Math.random() but with a seed I control.
  // Modulus is in parens to work around a Crunchinator bug.
  // This RNG is very poor (quickly repeats).  If for some reason we don't want to use the JS RNG we should put the 
  // Mersenne Twister in this file too.  See regexpfuzz.js.
  
  /*
  function rndReal() {
	seed = (seed*9301 + 49297) % (233280);
	return seed/(233280.0);
  };
  */
  
  function rndReal() { return Math.random(); }

  // Returns a somewhat random integer in the range {0, 1, 2, ..., number-1}
  function rndInt(number) {
	return Math.floor(rndReal() * number);
  };
  
  return rndInt;
}

var rnd = randomizer(randomSeed);

function rndElt(a) { return a[rnd(a.length)] }





// Each input to T should be a token or so, OR a bigger logical piece (such as a call to makeExpr).  Smaller than a token is ok too ;)

// When "torture" is true, T may do any of the following:
// * skip a token
// * skip all the tokens to the left
// * skip all the tokens to the right
// * insert unterminated comments
// * insert line breaks
// * insert entire expressions
// * insert any token

// Even when not in "torture" mode, it may sneak in extra line breaks.

// Why did I decide to toString at every step, instead of making larger and larger arrays (or more and more deeply nested arrays?).  no particular reason...

function Tmean(toks)
{
  var torture = (rnd(170) == 57);
  if (torture)
    dumpln("Torture!!!");
  
  var s = maybeLineBreak();
  var i;
  for (i=0; i<toks.length; ++i) {

    // Catch bugs in the fuzzer.  An easy mistake is
    //   return /*foo*/ + ...
    // instead of
    //   return "/*foo*/" + ...
    // Unary plus in the first one coerces the string that follows to number!
    if(typeof(toks[i]) != "string")
      alert("Strange toks[i]: " + toks[i]);
      
    if (!(torture && rnd(12) == 0))
      s += toks[i];
    
    s += maybeLineBreak();
    
    if (torture) switch(rnd(120)) {
      case 0: 
      case 1:
        s += maybeSpace() + rndElt(crazyTokens) + maybeSpace();
        break;
      case 2:
        s += maybeSpace() + makeTerm(2) + maybeSpace();
        break;
      case 3:
        s += maybeSpace() + makeExpr(2) + maybeSpace();
        break;
      case 4:
        s += maybeSpace() + makeStatement(2) + maybeSpace();
        break;
      case 5:
        s = "(" + s + ")"; // randomly parenthesize some prefix of it.
        break;
      case 6:
        s = ""; // throw away everything before this point
        break;
      case 7:
        return s; // throw away everything after this point
      case 8:
        s += UNTERMINATED_COMMENT;
        break;
      case 9:
        s += UNTERMINATED_STRING_LITERAL;
        break;
      case 10:
        if (rnd(2)) 
          s += "(";
        s += UNTERMINATED_REGEXP_LITERAL;
        break;
      default:
    }

  }

  return s;
}

// For reference and debugging, here's what a T without the torture stuff and extra line breaks would look like:
function Tnice(toks)
{
  var s = ""
  var i;
  for (i=0; i<toks.length; ++i) {
    if(typeof(toks[i]) != "string")
      alert("Strange toks[i]: " + toks[i]);

    s += toks[i];
  }

  return s;
}


T = Tmean;

var UNTERMINATED_COMMENT = "/*"; /* this comment is here so my text editor won't get confused */
var UNTERMINATED_STRING_LITERAL = "'";
var UNTERMINATED_REGEXP_LITERAL = "/";

function maybeLineBreak()
{ 
  if (rnd(300) == 3) 
    return "\n"; // line break to trigger semicolon insertion and stuff
  else
    return "";
}

function maybeSpace()
{
  if (rnd(2) == 0)
    return " ";
  else
    return "";
}

function stripSemicolon(c)
{
  var len = c.length;
  if (c.charAt(len - 1) == ";")
    return c.substr(0, len - 1);
  else
    return c;
}

function maybeLabel()
{
  if (rnd(4) == 1)
    return T([rndElt(["L", "M"]), ":"]);
  else
    return "";
}




function makeStatement(depth)
{
  var dr = rnd(depth); // instead of depth - 1;
  
  if (depth < rnd(8)) // frequently for small depth, infrequently for large depth
    return makeLittleStatement(dr);

  return (rndElt(statementMakers))(dr)
}

var varBinder = ["var ", "let ", "const ", ""];

var statementMakers = [
  // Late-defined consts can cause problems, so let's late-define them!
  function(dr) { return T([makeStatement(dr), " const ", makeId(dr), ";"]); },

  function(dr) { return T([makeStatement(dr), makeStatement(dr)]); },
  function(dr) { return T([makeStatement(dr-1), "\n", makeStatement(dr-1), "\n"]); },

  // Stripping semilcolons.  What happens if semicolons are missing?  Especially with line breaks used in place of semicolons (semicolon insertion).
  function(dr) { return T([stripSemicolon(makeStatement(dr)), "\n", makeStatement(dr)]); },
  function(dr) { return T([stripSemicolon(makeStatement(dr)), "\n"                   ]); },
  function(dr) { return stripSemicolon(makeStatement(dr)); }, // usually invalid, but can be ok e.g. at the end of a block with curly braces

  // Blocks and loops
  function(dr) { return T(["{", makeStatement(dr), " }"]); },
  function(dr) { return T(["{", makeStatement(dr-1), makeStatement(dr-1), " }"]); },

  function(dr) { return T([maybeLabel(), "with", "(", makeExpr(dr), ")", makeStatementOrBlock(dr)]); }, 
  function(dr) { return T([maybeLabel(), "with", "(", "{", makeId(dr), ": ", makeExpr(dr), "}", ")", makeStatementOrBlock(dr)]); }, 

  // C-style "for" loops
  // Two kinds of "for" loops: one with an expression as the first part, one with a var or let binding 'statement' as the first part.
  // I'm not sure if arbitrary statements are allowed there; I think not.
  function(dr) { return "/*noex infloop*/" + T([maybeLabel(), "for", "(", makeExpr(dr), "; ", makeExpr(dr), "; ", makeExpr(dr), ") ", makeStatementOrBlock(dr)]); }, 
  function(dr) { return "/*noex infloop*/" + T([maybeLabel(), "for", "(", rndElt(varBinder), makeId(dr), "; ", makeExpr(dr), "; ", makeExpr(dr), ") ", makeStatementOrBlock(dr)]); }, 

  // "for..in" loops
  // -- for (key in obj)
  function(dr) { return T([maybeLabel(), "for", "(", rndElt(varBinder), makeId(dr), " in ", makeExpr(dr-2), ") ", makeStatementOrBlock(dr)]); },
  // -- for (key in generator())
  function(dr) { return T([maybeLabel(), "for", "(", rndElt(varBinder), makeId(dr), " in ", "(", "(", makeFunction(dr), ")", "(", makeExpr(dr), ")", ")", ")", makeStatementOrBlock(dr)]); },
  // -- for each (value in obj)
  function(dr) { return "/* nobrowserex bug 349964 */" + T([maybeLabel(), " for ", " each", "(", rndElt(varBinder), makeId(dr), " in ", makeExpr(dr-2), ") ", makeStatementOrBlock(dr)]); },
  // -- for ([key, value] in obj), a special type of destructuring
  function(dr) { return T([maybeLabel(), " for ", "(", rndElt(varBinder), "[", makeId(dr), ", ", makeId(dr), "]", " in ", makeExpr(dr-2), ") ", makeStatementOrBlock(dr)]); },

  // Hoisty "for..in" loops.  I don't know why this construct exists, but it does, and it hoists the initial-value expression above the loop.
  // With "var" or "const", the entire thing is hoisted.
  // With "let", only the value is hoisted, and it can be elim'ed as a useless statement.
  function(dr) { return "/* HOISTY1 */" + T([maybeLabel(), "for", "(", rndElt(varBinder), makeId(dr), " = ", makeExpr(dr), " in ", makeExpr(dr-2), ") ", makeStatementOrBlock(dr)]); },

  // Commented out until Brendan responds...
  function(dr) { return "/* HOISTY2 */" + T([maybeLabel(), "for", "(", rndElt(varBinder), "[", makeId(dr), ", ", makeId(dr), "]", " = ", makeExpr(dr), " in ", makeExpr(dr-2), ") ", makeStatementOrBlock(dr)]); },

  function(dr) { return T([maybeLabel(), "while(0)" /*don't split this, it's needed to avoid noex*/, makeStatementOrBlock(dr)]); },
  function(dr) { return "/*noex*/" + T([maybeLabel(), "while", "(", makeExpr(dr), ")", makeStatementOrBlock(dr)]); },
  function(dr) { return T([maybeLabel(), "do ", makeStatementOrBlock(dr), " while(0)" /*don't split this, it's needed to avoid noex*/, ";"]); },
  function(dr) { return "/*noex*/" + T([maybeLabel(), "do ", makeStatementOrBlock(dr), " while", "(", makeExpr(dr), ");"]); },

  // Switch with some fallthroughs, default, etc.
  function(dr) { return T([maybeLabel(), "switch", "(", makeExpr(dr), ")", " { ", "case", " 0", ":", " case", " 1: ", makeStatement(dr), ";", "break", ";", " case", " 2", ": ", makeStatement(dr), ";", " case ", "3", ": ", makeStatement(dr), " ;", " default", ": ", makeStatement(dr), " }"]); },
  
  // Switch with expressions as cases, instead of simple constants.
  function(dr) { return T([maybeLabel(), "switch", "(", makeExpr(dr), ")", " { ", "case ", makeExpr(dr), ": ", makeStatement(dr), " case ", makeExpr(dr), ": ", makeStatement(dr), " }"]); },

  // Let blocks, with and without multiple bindings, with and without initial values
  function(dr) { return T(["let ", "(", makeId(dr), ")",                                        " { ", makeStatement(dr), " }"]); },
  function(dr) { return T(["let ", "(", makeId(dr), "=", "3", ", ", makeId(dr), "=", "4", ")",  " { ", makeStatement(dr), " }"]); },
  function(dr) { return T(["let ", "(", makeId(dr), "=", makeExpr(dr) + ")",                    " { ", makeStatement(dr), " }"]); },

  // Conditionals, perhaps with 'else if' / 'else'
  function(dr) { return T([maybeLabel(), "if(", makeExpr(dr), ") ", makeStatementOrBlock(dr)]); },
  function(dr) { return T([maybeLabel(), "if(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1), " else ", makeStatementOrBlock(dr-1)]); },
  function(dr) { return T([maybeLabel(), "if(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1), " else ", " if ", "(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1)]); },
  function(dr) { return T([maybeLabel(), "if(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1), " else ", " if ", "(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1), " else ", makeStatementOrBlock(dr-1)]); },

  // A tricky pair of if/else cases.
  // In the SECOND case, braces must be preserved to keep the final "else" associated with the first "if".
  function(dr) { return "/*TODE1*/" + T([maybeLabel(), "if(", makeExpr(dr), ") ", "{", " if ", "(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1), " else ", makeStatementOrBlock(dr-1), "}"]); },
  function(dr) { return "/*TODE2*/" + T([maybeLabel(), "if(", makeExpr(dr), ") ", "{", " if ", "(", makeExpr(dr), ") ", makeStatementOrBlock(dr-1), "}", " else ", makeStatementOrBlock(dr-1)]); },
  
  // Exception-related statements :)
  function(dr) { return makeExceptionyStatement(dr-1); makeExceptionyStatement(dr-1); },
  function(dr) { return makeExceptionyStatement(dr-1); makeExceptionyStatement(dr-1); },
  function(dr) { return makeExceptionyStatement(dr); },
  function(dr) { return makeExceptionyStatement(dr); },
  function(dr) { return makeExceptionyStatement(dr); },
  function(dr) { return makeExceptionyStatement(dr); },
  function(dr) { return makeExceptionyStatement(dr); },

  // Labels. (JavaScript does not have goto, but it does have break-to-label and continue-to-label).
  function(dr) { return T(["L", ": ", makeStatementOrBlock(dr)]); },
];


function makeLittleStatement(depth)
{
  var dr = depth - 1;

  if (rnd(4) == 1)
    return makeStatement(dr);
  
  return (rndElt(littleStatementMakers))(dr);
}

var littleStatementMakers = 
[
  // Tiny
  function(dr) { return T([";"]); }, // e.g. empty "if" block
  function(dr) { return T(["{", "}"]); ; }, // e.g. empty "if" block
  function(dr) { return T([""]); },

  // Force garbage collection
  // function(dr) { return "gc()"; },
  
  // Throw stuff.
  function(dr) { return T(["throw ", makeExpr(dr), ";"]); },

  // Break/continue [to label].
  function(dr) { return T([rndElt(["continue", "break"]), " ", rndElt(["L", "M", "", ""]), ";"]); },

  // Import and export.  (I have not idea what these actually do.)
  function(dr) { return T(["export ", makeId(dr), ";"]); },
  function(dr) { return T(["export ", "*", ";"]); },
  function(dr) { return T(["import ", makeId(dr), ".", makeId(dr), ";"]); },
  function(dr) { return T(["import ", makeId(dr), ".", "*", ";"]); },
  
  // Named and unnamed functions (which have different behaviors in different places: both can be expressions,
  // but unnamed functions "want" to be expressions and named functions "want" to be special statements)
  function(dr) { return makeFunction(dr); },
  
  // Return, yield
  function(dr) { return T(["return ", makeExpr(dr), ";"]); },
  function(dr) { return "return;"; }, // return without a value is allowed in generators; return with a value is not.
  function(dr) { return T(["yield ", makeExpr(dr), ";"]); }, // note: yield can also be a left-unary operator, or something like that
  function(dr) { return "yield;"; },

  // Expression statements
  function(dr) { return T([makeExpr(dr), ";"]); },
  function(dr) { return T(["(", makeExpr(dr), ")", ";"]); },
  
  // Various kinds of variable declarations, with and without initial values (assignment).
  function(dr) { return T([rndElt(varBinder), makeId(dr), ";"]); },
  function(dr) { return T([rndElt(varBinder), makeId(dr), " = ", makeExpr(dr), ";"]); },
  function(dr) { return T([rndElt(varBinder), makeLValue(dr), " = ", makeExpr(dr), ";"]); }, // e.g. "const [a,b] = [3,4];"
]


// makeStatementOrBlock exists because often, things have different behaviors depending on where there are braces.
// for example, if braces are added or removed, the meaning of "let" can change.
function makeStatementOrBlock(depth)
{
  var dr = depth - 1;
  return (rndElt(statementBlockMakers))(dr)
} 
var statementBlockMakers = [
  function(dr) { return makeStatement(dr); },
  function(dr) { return makeStatement(dr); },
  function(dr) { return T(["{", makeStatement(dr), " }"]); },
  function(dr) { return T(["{", makeStatement(dr-1), makeStatement(dr-1), " }"]); },
]


// Extra-hard testing for try/catch/finally and related things.

function makeExceptionyStatement(depth)
{
  var dr = depth - 1;
  if (dr < 1)
    return makeLittleStatement(dr);
    
  return (rndElt(exceptionyStatementMakers))(dr);
}

var exceptionyStatementMakers = [
  function(dr) { return makeTryBlock(dr); },

  function(dr) { return makeStatement(dr); },
  function(dr) { return makeLittleStatement(dr); },

  function(dr) { return "return;" }, // return without a value can be mixed with yield
  function(dr) { return T(["return ", makeExpr(dr), ";"]); },
  function(dr) { return T(["yield ", makeExpr(dr), ";"]); },
  function(dr) { return T(["throw ", makeId(dr), ";"]); },
  function(dr) { return "throw StopIteration;"; },
  function(dr) { return "this.zzz.zzz;"; }, // throws; also tests js_DecompileValueGenerator in various locations
  function(dr) { return T([makeId(dr), " = ", makeId(dr), ";"]); },
  function(dr) { return T([makeLValue(dr), " = ", makeId(dr), ";"]); },

  // Iteration uses StopIteration internally.
  // Iteration is also useful to test because it asserts that there is no pending exception.
  function(dr) { return "for(let y in []);"; }, 
  function(dr) { return "for(let y in [5,6,7,8]) " + makeExceptionyStatement(dr); }, 
  
  // Brendan says these are scary places to throw: with, let block, lambda called immediately in let expr.
  // And I think he was right.
  function(dr) { return "with({}) "   + makeExceptionyStatement(dr);         },
  function(dr) { return "with({}) { " + makeExceptionyStatement(dr) + " } "; },
  function(dr) { return "let(y) { " + makeExceptionyStatement(dr); + "}"},
  function(dr) { return "let(y) ((function(){" + makeExceptionyStatement(dr) + "})());" }
  
];

function makeTryBlock(depth)
{
  // Catches: 1/6 chance of having none
  // Catches: maybe 2 + 1/2 
  // So approximately 4 recursions into makeExceptionyStatement on average!
  // Therefore we want to keep the chance of recursing too much down...
  
  var dr = depth - rnd(3);
  

  var s = T(["try", " { ", makeExceptionyStatement(dr), " } "]);

  var numCatches = 0;

  var ct;
  
  while((ct = rnd(6)) < 2) {
    // Add a guarded catch, using an expression or a function call.
    ++numCatches;
    if (ct == 0)
      s += T(["catch", "(", makeId(dr), " if ",                 makeExpr(dr),                    ")", " { ", makeExceptionyStatement(dr), " } "]);
    else // ct == 1
      s += T(["catch", "(", makeId(dr), " if ", "(function(){", makeExceptionyStatement(dr), "})())", " { ", makeExceptionyStatement(dr), " } "]);
  }
  
  if (rnd(2)) {
    // Add an unguarded catch.
    ++numCatches;
    s +=   T(["catch", "(", makeId(dr),                                                          ")", " { ", makeExceptionyStatement(dr), " } "]);
  }
  
  if (numCatches == 0 || rnd(2) == 1) {
    // Add a finally.
    s += T(["finally", " { ", makeExceptionyStatement(dr), " } "]);
  }
  
  return s;
}



// Creates a string that sorta makes sense as an expression
function makeExpr(depth)
{
  if (depth <= 0 || (rnd(7) == 1))
    return makeTerm(depth - 1);

  var dr = rnd(depth); // depth - 1;

  var expr = (rndElt(exprMakers))(dr);
  
  if (rnd(4) == 1)
    return "(" + expr + ")";
  else
    return expr;
}

var binaryOps = [
  // Long-standing JavaScript operators, roughly in order from http://www.codehouse.com/javascript/precedence/
  " * ", " / ", " % ", " + ", " - ", " << ", " >> ", " >>> ", " < ", " > ", " <= ", " >= ", " instanceof ", " in ", " == ", " != ", " === ", " !== ",
  " & ", " | ", " ^ ", " && ", " || ", " = ", " *= ", " /= ", " %= ", " += ", " -= ", " <<= ", " >>= ", " >>>=", " &= ", " ^= ", " |= ", " , ",

  // . is special, so test it as a group of right-unary ops, a special exprMaker for property access, and a special exprMaker for the xml filtering predicate operator
  // " . ", 
  
  // Added by E4X
  " :: ", " .. ", " @ ",
  // Frequent combinations of E4X things (and "*" namespace, which isn't produced by this fuzzer otherwise)
  " .@ ", " .@*:: ", " .@x:: ",
];

var leftUnaryOps = [
  "--", "++", 
  "!", "+", "-", "~",
  "void ", "typeof ", "delete ", 
  "new ", // but note that "new" can also be a very strange left-binary operator
  "yield " // see http://www.python.org/dev/peps/pep-0342/ .  Often needs to be parenthesized, so there's also a special exprMaker for it.
];

var rightUnaryOps = [
  "++", "--",
  // E4X
  ".*", ".@foo", ".@*"
];

var specialProperties = [".prop", ".__iterator__", ".__parent__", ".__proto__"]
    

var exprMakers =
[
  // Left-unary operators
  function(dr) { return T([rndElt(leftUnaryOps), makeExpr(dr)]); },
  
  // Right-unary operators
  function(dr) { return T([makeExpr(dr), rndElt(rightUnaryOps)]); },
  function(dr) { return T([makeExpr(dr), rndElt(specialProperties)]); },
  
  // Binary operators
  function(dr) { return T([makeExpr(dr), rndElt(binaryOps), makeExpr(dr)]); },
  
  // Ternary operator
  function(dr) { return T([makeExpr(dr), " ? ", makeExpr(dr), " : ", makeExpr(dr)]); },

  // In most contexts, yield expressions must be parenthesized, so including explicitly parenthesized yields makes actually-compiling yields appear more often.
  function(dr) { return T(["(", "yield ", makeExpr(dr), ")"]); },
  
  // Array extras.  The most interesting are map and filter, I think.
  // These are mostly interesting to fuzzers in the sense of "what happens if i do strange things from a filter function?"  e.g. modify the array.. :)
  // This fuzzer isn't the best for attacking this kind of thing, since it's unlikely that the code in the function will attempt to modify the array or make it go away.
  // The second parameter to "map" is used as the "this" for the function.
  function(dr) { return T(["[11,12,13,14]",        ".", rndElt(["map", "filter", "some"]) ]); },
  function(dr) { return T(["[15,16,17,18]",        ".", rndElt(["map", "filter", "some"]), "(", makeFunction(dr), ", ", makeExpr(dr), ")"]); },
  function(dr) { return T(["[", makeExpr(dr), "]", ".", rndElt(["map", "filter", "some"]), "(", makeFunction(dr), ")"]); },
  
  // RegExp replace.  This is interesting for same same reason as array extras.
  function(dr) { return T(["'fafafa'", ".", "replace", "(", "/", "a", "/", "g", ", ", makeFunction(dr), ")"]); },

  // XML filtering predicate operator!
  function(dr) { return T([makeId(dr),  ".", "(", makeExpr(dr), ")"]); },
  function(dr) { return T([makeE4X(dr),  ".", "(", makeExpr(dr), ")"]); },

  // Dot (property access)
  function(dr) { return T([makeId(dr),    ".", makeId(dr)]); },
  function(dr) { return T([makeExpr(dr),  ".", makeId(dr)]); },

  // Index into array
  function(dr) { return T([     makeExpr(dr),      "[", makeExpr(dr), "]"]); },
  function(dr) { return T(["(", makeExpr(dr), ")", "[", makeExpr(dr), "]"]); },

  // Containment in an array or object (or, if this happens to end up on the LHS of an assignment, destructuring)
  function(dr) { return T(["[", makeExpr(dr), "]"]); },
  function(dr) { return T(["(", "{", makeId(dr), ": ", makeExpr(dr), "}", ")"]); },

  // Functions: called immediately/not
  function(dr) { return makeFunction(dr); },
  function(dr) { return T(["(", makeFunction(dr), ")", "(", makeActualArgList(dr), ")"]); },

  // Try to call things that may or may not be functions.
  function(dr) { return T([     makeExpr(dr),      "(", makeActualArgList(dr), ")"]); },
  function(dr) { return T(["(", makeExpr(dr), ")", "(", makeActualArgList(dr), ")"]); },

  // Binary "new", with and without clarifying parentheses, with empty and non-empty parens (note that empty parens for new usually decompile to zilch)
  function(dr) { return T(["new ",      makeExpr(dr),      "(", makeExpr(dr-1), ")"]); },
  function(dr) { return T(["new ", "(", makeExpr(dr), ")", "(", makeExpr(dr-1), ")"]); },
  function(dr) { return T(["new ",      makeExpr(dr),      "(",                 ")"]); },
  function(dr) { return T(["new ", "(", makeExpr(dr), ")", "(",                 ")"]); },

  // Sometimes we do crazy stuff, like putting a statement where an expression should go.  This frequently causes a syntax error.
  function(dr) { return stripSemicolon(makeLittleStatement(dr)); },
  function(dr) { return ""; },

  // Let expressions -- note the lack of curly braces.
  function(dr) { return T(["let", " (", "x", "=", "3", ",", "y", "=", "4", ") ", makeExpr(dr)]); },   
  function(dr) { return T(["let ", "(", makeId(dr), " = ", "3", ") ", makeExpr(dr)]); },
  function(dr) { return T(["let ", "(", makeId(dr), " = ", makeExpr(dr), ") ", makeExpr(dr)]); },
  
  // Array comprehensions
  function(dr) { return T(["[", makeExpr(dr), " for ",          "(", makeId(dr), " in ", makeExpr(dr-2), ")", "]"]); },
  function(dr) { return T(["[", makeExpr(dr), " for ", "each ", "(", makeId(dr), " in ", makeExpr(dr-2), ")", "]"]); },
      
  function(dr) { return T([" /* Comment */", makeExpr(dr)]); },
  function(dr) { return T(["\n", makeExpr(dr)]); }, // perhaps trigger semicolon insertion and stuff
  function(dr) { return T([makeExpr(dr), "\n"]); },

  function(dr) { return T([makeLValue(dr)]); },

  // Assignment (can be destructuring)
  function(dr) { return T([makeLValue(dr), " = ", makeExpr(dr)]); },
  function(dr) { return T([makeLValue(dr), " = ", makeExpr(dr)]); },
  function(dr) { return T([makeLValue(dr), " = ", makeExpr(dr)]); },
  function(dr) { return T([makeLValue(dr), " = ", makeExpr(dr)]); },

  // Destructuring assignment
  function(dr) { return T([makeDestructuringLValue(dr), " = ", makeExpr(dr)]); },
  function(dr) { return T([makeDestructuringLValue(dr), " = ", makeExpr(dr)]); },
  function(dr) { return T([makeDestructuringLValue(dr), " = ", makeExpr(dr)]); },
  function(dr) { return T([makeDestructuringLValue(dr), " = ", makeExpr(dr)]); },
  
  // Modifying assignment, with operators that do various coercions
  function(dr) { return T([makeLValue(dr), rndElt(["|=", "%=", "+=", "-="]), makeExpr(dr)]); },
  
  // Object literal with (newish-style?) getter and setter
  function(dr) { return T(["(", "{", " get ", makeId(dr), "(", makeFormalArgList(dr-1), ")", " { " , makeStatement(dr-1), " }", ",", " set ", makeId(dr), "(", makeFormalArgList(dr-1), ")", " { ", makeStatement(dr-1), "}", " }", ")"]); },

  // Object literal with obscure old-style getter and setter
  // commented out due to comments late in bug 352455
  //function(dr) { return T(["(", "{", makeId(dr), " getter: ", makeFunction(dr), " }", ")"]); },
  //function(dr) { return T(["(", "{", makeId(dr), " setter: ", makeFunction(dr), " }", ")"]); },

  // Old-style getter/setter
  function(dr) { return T([makeId(dr), ".", makeId(dr), " ", rndElt(["getter", "setter"]), "= ", makeFunction(dr)]); },

  // Try to cause trouble using toString and valueOf.
  function(dr) { return T(["({y:5, ", "toString: ", makeFunction(dr), "}", ")"]); },
  function(dr) { return T(["({y:5, ", "toString: function() { return this; } }", ")"]); }, // bwahaha
  function(dr) { return T(["({y:5, ", "toString: function() { return " + makeExpr(dr) + "; } }", ")"]); },
  // Was commented out to avoid bug 352742.  (This is an execution bug, but compilation and decompilation are tested just fine by the toString stuff above.)
  function(dr) { return T(["({y:5, ", "valueOf: ", makeFunction(dr), "}", ")"]); },
  function(dr) { return T(["({y:5, ", "valueOf: function() { return " + makeExpr(dr) + "; } }", ")"]); },

  // Try to test function.call heavily.
  function(dr) { return T(["(", makeFunction(dr), ")", ".", "call", "(", makeExpr(dr), ", ", makeExpr(dr), ")"]); },
  
  // Test js_ReportIsNotFunction heavily.
  function(dr) { return "(p={}, (p.z = " + makeExpr(dr) + ")())"; },

  // Test js_ReportIsNotFunction heavily.
  // Test decompilation for ".keyword" a bit.
  // Test throwing-into-generator sometimes.
  function(dr) { return T([makeExpr(dr), ".", "throw", "(", makeExpr(dr), ")"]); },
  function(dr) { return T([makeExpr(dr), ".", "pow",   "(", makeExpr(dr), ")"]); },

  // Throws, but more importantly, tests js_DecompileValueGenerator in various contexts.
  function(dr) { return "this.zzz.zzz"; }, 
  
  // Test obj.eval stuff carefully (but avoid clobbering eval)
  function(dr) { return makeExpr(dr) + ".eval(" + makeExpr(dr) + ")"; },
//  function(dr) { return "eval(" + uneval(makeExpr(dr)) + ", " + makeExpr(dr) + ")"; },      // eval an actual expression! how clever!
//  function(dr) { return "eval(" + uneval(makeStatement(dr)) + ", " + makeExpr(dr) + ")"; }, // eval an actual statement!  how clever!
  
  // Uneval needs more testing than it will get accidentally.  No T() because I don't want uneval clobbered (assigned to).
//  function(dr) { return "(uneval(" + makeId(dr) + "))"; },

  // Constructors.  No "T" -- don't screw with the constructors themselves; just call them.
  // Was commented out for bug 352742 (?)
  function(dr) { return "/* CONSTRUCT */ new " + rndElt(constructors) + "(" + makeActualArgList(dr) + ")"; }


];

var constructors = [
  /* "Error", "RangeError", "String", "Function", "Array", "Object" https://bugzilla.mozilla.org/show_bug.cgi?id=352742#c16 */ 
  "Iterator"
];


function makeFunction(depth)
{
  var dr = depth - 1;
  
  if(rnd(5) == 1)
    return makeExpr(dr);

  var y = [
    // Note that a function with a name is sometimes considered a statement rather than an expression.

    // Unnamed (lambda, generator)
    function(dr) { return T(["function", " (", makeFormalArgList(dr), ")", " { ", makeStatement(dr), " }"]); },
    function(dr) { return T(["function", " (", makeFormalArgList(dr), ")", " { ", "return ", makeExpr(dr), " }"]); },
    function(dr) { return T(["function", " (", makeFormalArgList(dr), ")", " { ", "yield ",  makeExpr(dr), " }"]); },

    // Named
    function(dr) { return T(["function ", makeId(dr), "(", makeFormalArgList(dr), ")", " { ", makeStatement(dr), " }"]); }, 

    // The identity function
    function(dr) { return "function(id2) { return id2; }" },

    // A generator that does something
    function(dr) { return "function(y) { yield y; " + makeStatement(dr) + "; yield y; }" }, 
    
    // Special functions that might have interesting results, especially when called "directly" by things like string.replace or array.map.
    function(dr) { return "eval" }, // eval is interesting both for its "no indirect calls" feature and for the way it's implemented -- a special bytecode.
    function(dr) { return "new Function" }, // this won't be interpreted the same way for each caller of makeFunction, but that's ok
    function(dr) { return "Function" }, // without "new"!  it does seem to work...
    function(dr) { return "gc" },
    function(dr) { return "Math.sin" },
    function(dr) { return "/a/gi" }, // in Firefox, at least, regular expressions can be used as functions: e.g. "hahaa".replace(/a+/g, /aa/g) is hnullhaa.
    function(dr) { return "[1,2,3,4].map" },
    function(dr) { return "[1,2,3,4].slice" },
    function(dr) { return "'haha'.split" },
    function(dr) { return T(["(", makeFunction(dr), ")", ".", "call"]); },
    function(dr) { return T(["(", makeFunction(dr), ")", ".", "apply"]); },
  ];
  
  return (rndElt(y))(dr);
}

function makeActualArgList(depth)
{
  var nArgs = rnd(3);

  if (nArgs == 0)
    return "";

  var argList = makeExpr(depth);

  for (var i = 1; i < nArgs; ++i)
    argList += ", " + makeExpr(depth - i);

  dumpln("Actual argList: " + argList);
  return argList;
}

function makeFormalArgList(depth)
{
  var nArgs = rnd(3);

  if (nArgs == 0)
    return "";

  var argList = makeFormalArg(depth)

  for (var i = 1; i < nArgs; ++i)
    argList += ", " + makeFormalArg(depth - i);
    
  dumpln("Formal argList: " + argList);
  return argList;
}

function makeFormalArg(depth)
{
  if (rnd(4) == 1)
    return makeDestructuringLValue(depth);
  
  return makeId(depth);
}


function makeId(dr) 
{
  switch(rnd(200))
  {
  case 0:
    return makeTerm(dr);
  case 1:  
    return makeExpr(dr);
  case 2: case 3: case 4: case 5:  
    return makeLValue(dr);
  case 6: case 7:
    return makeDestructuringLValue(dr);
  case 8: case 9: case 10: case 11: case 12:
    // some keywords that can be used as identifiers in some contexts (e.g. variables, function names, argument names)
    // but that's annoying, and some of these cause lots of syntax errors.
    return rndElt(["get", "set", "getter", "setter", "delete", "let", "yield", "each"]);
  }

  return rndElt(["x", "x", "x", "x", "x", "x", "x", "x", // repeat "x" so it's likely to be bound more than once, causing "already bound" errors, elimination of assign-to-const, or conflicts
                 "y", "window", "this", "\u3056", "NaN"
                  ]);
  // window is a const, so some attempts to redeclare it will cause errors

  // eval is interesting because it cannot be called indirectly. and maybe also because it has its own opcode in jsopcode.tbl.
  // but bad things happen if you have "eval setter"... so let's not put eval in this list.
}


function makeLValue(depth)
{
  if (depth <= 0 || (rnd(7) == 1))
    return makeId(depth - 1);

  var dr = rnd(depth);

  return (rndElt(lvalueMakers))(dr);
}


var lvalueMakers = [
  // Simple variable names :)
  function(dr) { return T([makeId(dr)]); },

  // Destructuring
  function(dr) { return makeDestructuringLValue(dr); },
  function(dr) { return makeDestructuringLValue(dr); },
  
  // Properties
  function(dr) { return T([makeId(dr), ".", makeId(dr)]); },
  function(dr) { return T([makeExpr(dr), ".", makeId(dr)]); },
  function(dr) { return T([makeExpr(dr), "[", "'", makeId(dr), "'", "]"]); },

  // Special properties
  function(dr) { return T([makeId(dr), rndElt(specialProperties)]); },

  // Certain functions can act as lvalues!  See JS_HAS_LVALUE_RETURN in js engine source.
  function(dr) { return T([makeId(dr), "(", makeExpr(dr), ")"]); },
  function(dr) { return T(["(", makeExpr(dr), ")", "(", makeExpr(dr), ")"]); },

  // Parenthesized lvalues can cause problems ;)
  function(dr) { return T(["(", makeLValue(dr), ")"]); },

  function(dr) { return makeExpr(dr); } // intentionally bogus, but not quite garbage.
];

function makeDestructuringLValue(depth)
{
  var dr = depth - 1;

  if (dr < 0 || rnd(4) == 1)
    return makeId(dr);

  if (rnd(6) == 1)
    return makeLValue(dr);

  return (rndElt(destructuringLValueMakers))(dr);
}

var destructuringLValueMakers = [
  // destructuring assignment: arrays
  function(dr) { return T(["[", makeDestructuringLValue(dr), "]"]); },
  function(dr) { return T(["[", makeDestructuringLValue(dr), ", ", makeDestructuringLValue(dr), "]"]); },

  // destructuring assignment: objects
  function(dr) { return T(["(", "{ ", makeId(dr), ": ", makeDestructuringLValue(dr), " }", ")"]); },
  function(dr) { return T(["(", "{ ", makeId(dr), ": ", makeDestructuringLValue(dr), ", ", makeId(dr), ": ", makeDestructuringLValue(dr), " }", ")"]); },
];



function makeTerm(dr)
{

  var y = [
    // Variable names
    function(dr) { return makeId(dr); },

    // Simple literals (no recursion required to make them)
    function(dr) { return rndElt([ 
      "[]", "[1]", "[[]]", "[[1]]",
      "{}", "({})", "({a1:1})", 
      "[z1]", // can be interpreted as destructuring
      "({a2:z2})", // can be interpreted as destructuring
      "function(id) { return id }",
      "function ([y]) { }",
      "(function ([y]) { })()"
      ]);
    },
    function(dr) { return rndElt([ "0.1", ".2", "3", "1.3", "4.", "5.0000000000000000000000", "1e81", "1e+81", "1e-81", "1e4", "0", "-0", "(-0)", "0x99", "033", (""+Math.PI), "3/0", "-3/0", "0/0"]); },
    function(dr) { return rndElt([ "true", "false", "undefined", "null"]); },
    function(dr) { return rndElt([ "this", "window" ]); },
    function(dr) { return rndElt([" \"\" ", " '' ", " /x/ ", " /x/g "]) },

    // E4X literals
    function(dr) { return rndElt([ "<x/>", "<y><z/></y>"]); },
    function(dr) { return rndElt([ "@foo" /* makes sense in filtering predicates, at least... */, "*", "*::*"]); },
    function(dr) { return makeE4X(dr) }, // xml
    function(dr) { return T(["<", ">", makeE4X(dr), "<", "/", ">"]); }, // xml list
  ];
  
  return (rndElt(y))(dr);

}

function maybeMakeTerm(depth)
{
  if (rnd(2))
    return makeTerm(depth - 1);
  else
    return "";
}


var crazyTokens = 
[
// Some of this is from reading jsscan.h.

  // Comments; comments hiding line breaks.
  "//", UNTERMINATED_COMMENT, (UNTERMINATED_COMMENT + "\n"), "/*\n*/", 
  
  // groupers (which will usually be unmatched if they come from here ;)
  "[", "{", "]", "}", "(", ")",
  
  // a few operators
  "!", "@", "%", "^", "*", "|", ":", "?", "'", "\"", ",", ".", "/", 
  "~", "_", "+", "=", "-", "++", "--", "+=", "%=", "|=", "-=", 
  
  // most real keywords plus a few reserved keywords
  " in ", " instanceof ", " let ", " new ", " get ", " for ", " if ", " else ", " else if ", " try ", " catch ", " finally ", " export ", " import ", " void ", " with ", 
  " default ", " goto ", " case ", " switch ", " do ", " /*noex*/while ", " return ", " yield ", " break ", " continue ", " typeof ", " var ", " const ", 
  " enum ", // JS_HAS_RESERVED_ECMA_KEYWORDS
  " debugger ", // JS_HAS_DEBUGGER_KEYWORD
    
  " super ", // TOK_PRIMARY!
  " this ", // TOK_PRIMARY!
  " null ", // TOK_PRIMARY!
  " undefined ", // not a keyword, but a default part of the global object
  "\n", // trigger semicolon insertion, also acts as whitespace where it might not be expected
  "\r", 
  "\u2028", // LINE_SEPARATOR?
  "\u2029", // PARA_SEPARATOR?
  "<" + "!" + "--", // beginning of HTML-style to-end-of-line comment (!)
  "--" + ">", // end of HTML-style comment
  "",
  "\0", // confuse anything that tries to guess where a string ends. but note: "illegal character"!
];


function makeE4X(depth)
{
  if (depth <= 0)
    return T(["<", "x", ">", "<", "y", "/", ">", "<", "/", "x", ">"]);
    
  var dr = depth - 1;
  
  var y = [
    function(dr) { return '<employee id="1"><name>Joe</name><age>20</age></employee>' },
    function(dr) { return T(["<", ">", makeSubE4X(dr), "<", "/", ">"]); }, // xml list

    function(dr) { return T(["<", ">", makeExpr(dr), "<", "/", ">"]); }, // bogus or text
    function(dr) { return T(["<", "zzz", ">", makeExpr(dr), "<", "/", "zzz", ">"]); }, // bogus or text
    
    // mimic parts of this example at a time, from the e4x spec: <x><{tagname} {attributename}={attributevalue+attributevalue}>{content}</{tagname}></x>;

    function(dr) { var tagId = makeId(dr); return T(["<", "{", tagId, "}", ">", makeSubE4X(dr), "<", "/", "{", tagId, "}", ">"]); },
    function(dr) { var attrId = makeId(dr); var attrValExpr = makeExpr(dr); return T(["<", "xxx", " ", "{", attrId, "}", "=", "{", attrValExpr, "}", " ", "/", ">"]); },
    function(dr) { var contentId = makeId(dr); return T(["<", "xxx", ">", "{", contentId, "}", "<", "/", "xxx", ">"]); },
    
    // namespace stuff
    function(dr) { var contentId = makeId(dr); return T(['<', 'bbb', ' ', 'xmlns', '=', '"', makeExpr(dr), '"', '>', makeSubE4X(dr), '<', '/', 'bbb', '>']); },
    function(dr) { var contentId = makeId(dr); return T(['<', 'bbb', ' ', 'xmlns', ':', 'ccc', '=', '"', makeExpr(dr), '"', '>', '<', 'ccc', ':', 'eee', '>', '<', '/', 'ccc', ':', 'eee', '>', '<', '/', 'bbb', '>']); },
    
    function(dr) { return makeExpr(dr); },
    
    function(dr) { return makeSubE4X(dr); }, // naked cdata things, etc.
  ]
  
  return (rndElt(y))(dr);
}

function makeSubE4X(depth)
{
  if (rnd(8) == 0)
    return "<" + "!" + "[" + "CDATA[" + makeExpr(depth - 1) + "]" + "]" + ">"

  if (depth < -2)
    return "";

  var y = [
    function(depth) { return T(["<", "ccc", ":", "ddd", ">", makeSubE4X(depth - 1), "<", "/", "ccc", ":", "ddd", ">"]); },
    function(depth) { return makeE4X(depth) + makeSubE4X(depth - 1); },
    function(depth) { return "yyy"; },
    function(depth) { return T(["<", "!", "--", "yy", "--", ">"]); }, // XML comment
    function(depth) { return T(["<", "!", "[", "CDATA", "[", "zz", "]", "]", ">"]); }, // XML cdata section
    function(depth) { return " "; },
    function(depth) { return ""; },
  ];
  
  return (rndElt(y))(depth);
}



function start()
{
  count = 0;

  if (jsshell) {
    // Number of iterations: 20000 is good for use with multi_timed_run.py.  (~40 seconds, reduction isn't bad.)
    // Raise for use without multi_timed_run.py (perhaps to Infinity).
    // Lower for use with WAY_TOO_MUCH_GC.
    for (var i = 0; i < runCount; ++i)
      testOne();
    dumpln("It's looking good!"); // Magic string that multi_timed_run.py looks for
  }
  else {
    setTimeout(testStuffForAWhile, 200);
  }
}

function testStuffForAWhile()
{
  for (var j = 0; j < 200; ++j)
    testOne();

  dumpln("...");
  dumpln("");

  setTimeout(testStuffForAWhile, 200);
}

function testOne()
{
  
  ++count;

  var code = makeStatement(8);

  if (haveUneval)
    dumpln("count=" + count + "; tryItOut(" + uneval(code) + ");");

  tryItOut(code);
}

function tryItOut(code)
{
  var codeWithoutLineBreaks = code.replace(/\n/g, " ").replace(/\r/g, " "); // regexps can't match across lines
  var allowDecompile, checkRecompiling, checkForMismatch, allowExec, allowIter, checkLetScope;
  

  // This section is mostly exclusions for known Spidermonkey bugs.

  // Things like "noex" can appear in literal strings or comments.
  
  // Don't exec things that are likely infinite loops, or have known bugs when executing.
  // Don't decompile things that have known bugs when decompiling, etc.

  // Exclude things here if decompiling crashes.  
  
  allowDecompile =   (code.indexOf("nodecompile") == -1)
    ;
  
  // Exclude things here if decompiling returns something bogus that won't compile.
  
  checkRecompiling = (code.indexOf("ignoredecompilation") == -1)
    && !( codeWithoutLineBreaks.match( /for.*\(.*in.*const/ )) // avoid bug 352083, with for loops or array comprehensions
    && !( codeWithoutLineBreaks.match( /case.*yield.*\:/ )) // avoid the two forms of bug 352441
    && !( codeWithoutLineBreaks.match( /L\:.*let/ )) // avoid bug 352732 (and comment 1 there)
    && (code.indexOf("eval") == -1)    // avoid bug 352453
    && (code.indexOf("HOISTY") == -1 || code.indexOf(":") == -1) // avoid bug 352921 (i hope this is correct)      
    ;

  // Exclude things here if decompiling returns something incorrect or suboptimal or non-canonical, but that will compile.
  
  checkForMismatch = (code.indexOf("mismatchok") == -1)
    && !( codeWithoutLineBreaks.match( /\<.*\{/ )) // avoid bug 351706, which won't be fixed for a while :(
    && !( codeWithoutLineBreaks.match( /new.*\.\@/ )) // avoid bug 352789
    && !( codeWithoutLineBreaks.match( /const.*if/ )) // avoid bug 352985
    && !( codeWithoutLineBreaks.match( /if.*const/ )) // avoid bug 352985
    && (code.indexOf("delete") == -1)  // avoid bug 352027, which won't be fixed for a while :(
    && (code.indexOf("CDATA") == -1) // avoid bug 352285
    && (code.indexOf("<!") == -1)    // avoid bug 352285
    && (code.indexOf("<?") == -1)    // avoid bug 352285
    && (code.indexOf("import") == -1)    // avoid bug 350681
    && (code.indexOf("export") == -1)    // avoid bug 350681
    && (code.indexOf("new") == -1)    // avoid bug 353146
    && (code.indexOf("-3/0") == -1)    // avoid bug 351219 ?  negation creeps away
    && (code.indexOf("3/0") == -1)    // avoid thing where function() { return <x/>.(0/0) } changes parens during round trip, probably just more of bug 351219
    && (code.indexOf("0/0") == -1)    // avoid thing where function() { return <x/>.(0/0) } changes parens during round trip, probably just more of bug 351219

    && (code.indexOf("%")  == -1) // avoid bug 352085 -- common
    && (code.indexOf(">>") == -1) // avoid bug 352085 -- somewhat common
    && (code.indexOf("<<") == -1) // avoid bug 352085 -- somewhat common

    && (code.indexOf("/")  == -1 || (code.indexOf("\"") == -1 && code.indexOf("\'") == -1) ) // avoid bug 352085 - annoying
    && (code.indexOf("|")  == -1 || (code.indexOf("\"") == -1 && code.indexOf("\'") == -1) ) // avoid bug 352085 - annoying
    && (code.indexOf("*")  == -1 || (code.indexOf("\"") == -1 && code.indexOf("\'") == -1) ) // avoid bug 352085
    && (code.indexOf("-")  == -1 || (code.indexOf("\"") == -1 && code.indexOf("\'") == -1) ) // avoid bug 352085
    && (code.indexOf("~")  == -1 || (code.indexOf("\"") == -1 && code.indexOf("\'") == -1) ) // avoid bug 352085

    // Mostly the same tests as for checkLetScope, hmm.
    && !( codeWithoutLineBreaks.match( /do.*let.*while/ ))   // avoid bug 352421
    && !( codeWithoutLineBreaks.match( /with.*let/ )) // avoid rebracing issues that fall out of bug 352422
    && !( codeWithoutLineBreaks.match( /for.*let.*\).*function/ )) // avoid bug 352735 (more rebracing stuff)
    && !( codeWithoutLineBreaks.match( /if\s*\(.*\).*let/ )) // avoid bug 352786
    ;
    
  checkLetScope = false // this test is slow :(
    && (code.indexOf("ignoreletscope") == -1)
    // Shared...
    && !( codeWithoutLineBreaks.match( /do.*let.*while/ )) // avoid bug 352421
    && !( codeWithoutLineBreaks.match( /with.*let/ )) // avoid rebracing issues that fall out of bug 352422
    && !( codeWithoutLineBreaks.match( /(for|while).*(for|while).*let/ )) // avoid bug 352907
    && !( codeWithoutLineBreaks.match( /if\s*\(.*\).*let/ )) // avoid bug 352786

    // Only for checkLetScope...
    && !( codeWithoutLineBreaks.match( /for.*let.*\).*function/ )) // avoid bug 352735 (more rebracing stuff)
    ;

  allowExec = code.indexOf("noex")        == -1
    && (jsshell || code.indexOf("nobrowserex") == -1)
    && !( codeWithoutLineBreaks.match( /const.*for/ )) // can be an infinite loop: function() { const x = 1; for each(x in ({a1:1})) print(3); }
    && !( codeWithoutLineBreaks.match( /for.*const/ )) // can be an infinite loop: for each(x in ...); const x;
    && !( codeWithoutLineBreaks.match( /for.*in.*uneval/ )) // can be slow to loop through the huge string uneval(this), for example
    && !( codeWithoutLineBreaks.match( /delete.*Function/ )) // avoid bug 352604 (exclusion needed despite the realFunction stuff?!)
    && !( codeWithoutLineBreaks.match( /for.*for.*for.*for.*for/ )) // nested for loops (array comprehensions, etc) can take a while
    && !( codeWithoutLineBreaks.match( /eval.*return/ ))   // avoid bug 352986
    && !( codeWithoutLineBreaks.match( /eval.*yield/ ))    // avoid bug 352986
    && !( codeWithoutLineBreaks.match( /eval.*break/ ))    // avoid bug 352986
    && !( codeWithoutLineBreaks.match( /eval.*continue/ )) // avoid bug 352986
    ;

  allowIter = code.indexOf("noiter")     == -1
    && typeof Iterator == "function" // false in e.g. Safari and older versions of Firefox
    ;
    
  
  if(verbose)
    dumpln("Verbose, count: " + count);
  
  if(verbose)
    dumpln("allowExec=" + allowExec + ", allowDecompile=" + allowDecompile + ", checkRecompiling=" + checkRecompiling + ", checkForMismatch=" + checkForMismatch + ", allowIter=" + allowIter + ", checkLetScope=" + checkLetScope);

  tryHalves(code);
  
  var f = null;

  try {
    // Try two methods of creating functions, just in case there are differences.
    if (count % 2 == 0 && allowExec) {
      if (verbose)
        dumpln("About to compile, using eval hack.")
      f = eval("$=function(){" + code + "};"); // Disadvantage: "}" can "escape", allowing code to *execute* that we only intended to compile.  Hence the allowExec check.
    }
    else {
      if (verbose)
        dumpln("About to compile, using new Function.")
      f = new Function(code);
    }
  } catch(compileError) {
    dumpln("Compiling threw: " + errorToString(compileError));
  }
  
  if (f && allowDecompile) { 
    if (verbose)
      dumpln("About to do the 'toString' round-trip test");
    checkRoundTrip2(f, code, checkRecompiling, checkForMismatch); // i like line breaks, so this one comes first
    if (haveUneval) {
      if (verbose)
        dumpln("About to do the 'uneval' round-trip test");
      checkRoundTrip(f, code, checkRecompiling, checkForMismatch);
    }
  }
  
  if (f && allowDecompile && checkRecompiling && checkLetScope) {
    if (verbose)
      dumpln("About to do the 'let x;' test");
    checkLetX(code);
    checkLetX(extractCode(f));
  }
  
  var rv = null;

  if (allowExec && f) {
    try { 
      // var e = eval(code);
      if (verbose)
        dumpln("About to run it!");
      rv = f();
      if (verbose)
        dumpln("It ran!"); // Keeping this because I'm confused!
    } catch(runError) {
       if(verbose)
         dumpln("Running threw!  About to toString to error.");
      dumpln("Running threw: " + errorToString(runError)); 
    }
  }
  
  allowIter = allowIter && rv && typeof rv == "object";

  if (allowIter) {
    try {
      allowIter = (Iterator(rv) === rv)
    }
    catch(e) {
      // Is it a bug that it's possible to end up here?  Probably not!
      allowIter = false;
      dumpln("Error while trying to determine whether it's an iterator!");
      dumpln("The error was: " + e);
    }
  }
  
  if (allowIter) {
    dumpln("It's an iterator!");
    try {
      var iterCount = 0;
      var iterValue;
      // To keep Safari-compatibility, don't use "let", "each", etc.
      for /* each */ ( /* let */ iterValue in rv)
        ++iterCount;
      dumpln("Iterating succeeded, iterCount == " + iterCount);
    } catch (iterError) {
      dumpln("Iterating threw!");
      dumpln("Iterating threw: " + errorToString(iterError));
    }
  }
  
  // Restore important stuff that might have been broken as soon as possible :)
  eval = realEval;
  Function = realFunction;
  gc = realGC;
  if (count % 1000 == 0) {
    dumpln("Paranoid GC!")
    dumpln(count);
    realGC();
  }
  
  if (!eval)
    dumplnAndAlert("OMGWTFBBQ");
    
  if (eval != realEval)
    dumplnAndAlert("WTFWTFWTF")

  if(verbose)
    dumpln("Done trying out that function!");
    
  dumpln("");
}

function tryHalves(code)
{
  // See if there are any especially horrible bugs that appear when the parser has to start/stop in the middle of something. this is kinda evil.

  // Stray "}"s are likely in secondHalf, so use new Function rather than eval.  "}" can't escape from new Function :)
  
  try {
    
    firstHalf = code.substr(0, code.length / 2);
    if (verbose)
      dumpln("First half: " + firstHalf);
    f = new Function(firstHalf); 
    uneval(f);
  }
  catch(e) { 
    if (verbose)
      dumpln("First half compilation error: " + e); 
  }

  try {
    secondHalf = code.substr(code.length / 2, code.length);
    if (verbose)
      dumpln("Second half: " + secondHalf);
    f = new Function(secondHalf);
    uneval(f);
  }
  catch(e) {
    if (verbose)
      dumpln("Second half compilation error: " + e);   
  }
}

function errorToString(e)
{
  try {
    return ("" + e);
  } catch (e2) {
    return "Can't toString the error!!";
  }
}



// Round-trip with uneval
function checkRoundTrip(f, code, checkRecompiling, checkForMismatch)
{
  var g, uf, ug;
  try {
    uf = uneval(f);
  } catch(e) { reportRoundTripIssue("Round-trip with uneval: can't uneval", code, null, null, errorToString(e)); return; }

  if (checkRecompiling) {
    try {
      g = eval("$=" + uf);
      ug = uneval(g);
      if (checkForMismatch && ug != uf) {
        reportRoundTripIssue("Round-trip with uneval: mismatch", code, uf, ug, "mismatch");
      }
    } catch(e) { reportRoundTripIssue("Round-trip with uneval: error", code, uf, ug, errorToString(e)); }
  }
}

// Round-trip with implicit toString
function checkRoundTrip2(f, code, checkRecompiling, checkForMismatch)
{
  var uf, g;
  try {
    uf = "" + f;
  } catch(e) { reportRoundTripIssue("Round-trip with implicit toString: can't toString", code, null, null, errorToString(e)); return; }

  if (checkRecompiling) {
    try {
      g = eval("$=" + uf);
      if (checkForMismatch && (""+g) != (""+f) ) {
        reportRoundTripIssue("Round-trip with implicit toString", code, f, g, "mismatch");
      }
    } catch(e) { reportRoundTripIssue("Round-trip with implicit toString: error", code, f, g, errorToString(e)); }
  }
}

// Round-trip mismatches or errors usually indicate that the first uneval was WRONG.

function reportRoundTripIssue(issue, code, fs, gs, e)
{
  var message = issue + "\n\n" +
                "Code: " + code + "\n\n" +
                "fs: " + fs + "\n\n" +
                "gs: " + gs + "\n\n" +
                "error: " + e;

  dumplnAndAlert(message);
}



function checkLetX(code)
{
  var letX = "\n;\nlet x;";
  var hasTopLevelLet = !compiles(code + letX);
  var f = new Function(code);
  var ef = extractCode(f);
  var getsTopLevelLet = !compiles(ef + letX);
  if (hasTopLevelLet != getsTopLevelLet) {
    var desc = "hasTopLevelLet == " + hasTopLevelLet + " but getsTopLevelLet == " + getsTopLevelLet;
    reportRoundTripIssue("checkLetX found that " + desc, code, ef, null, null);
  }
}

function extractCode(f)
{
  // throw away the first and last lines of the function's string representation
  // (this happens to work on spidermonkey trunk, dunno about anywhere else)
  var uf = "" + f;
  var lines = uf.split("\n");
  var innerLines = lines.slice(1, -1);
  return innerLines.join("\n");
}

function compiles(code)
{
  try {
    new Function(code);
    return true;
  } catch(e) {
    return false;
  }
}
    
  





var count;
var verbose;

/**************************************
 *  To reproduce a crash / assertion: *
 **************************************/

// 1. Comment "start();" out.
verbose = true;
start();


// 2. Uncomment this and paste a line from the run's output (count=..., tryItOut(...)) below this line.
verbose = true;




// 3. Run it.


// 4. If that doesn't work, maybe you're unlucky enough to have triggered a GC-related bug, or a bug that requires executing more than one of the randomly generated functions.  
// In that case, grep console output for tryItOut and paste it all in, then reduce using your favorite reduction method/tool.
