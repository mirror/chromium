/*
  This micro-benchmark is adapted from:
  
  http://wd-testnet.world-direct.at/mozilla/dhtml/funo/jsTimeTest.htm

*/

function Element(name) {
  this.name = name;
}

Element.prototype.appendChild = function(string) {
  print(this.name + ': ' + string);
};


var document = {};

document.getElementById = function(name) {
  return new Element(name);
};

document.createTextNode = function(string) {
  return string;
}

function completed() {
  // ignore
}


// -------------------------------------------------------------------
var start;
var end;
var forLoopTime = 0;
var addTime = 0;
var subtractTime = 0;
var multiplyTime = 0;
var divideTime = 0;
var divide2Time = 0;
var fromArrayTime = 0;
var parseIntTime = 0;
var varTime = 0;
var sinTime = 0;
var floorTime = 0;
var ifTime = 0;
var readGlobalTime = 0;
var concatStringsTime = 0;
var sortArrayTime = 0;

gValue = 123;

var continueAfter = false;

function testFor()
{
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    void(0);
  }
  end = new Date();
  forLoopTime = end-start;
  var textElem = document.createTextNode(forLoopTime + 'ms  ');
  document.getElementById('forResult').appendChild(textElem);
  completed();
}

function testAdd()
{
  var value1=123;
  var value2=234;
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = value1 + value2;
  }
  end = new Date();
  addTime = end-start;
  var textElem = document.createTextNode(addTime + 'ms  ');
  document.getElementById('addResult').appendChild(textElem);
  completed();
}

function testSubtract()
{
  var value1=123;
  var value2=234;
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = value2 - value1;
  }
  end = new Date();
  subtractTime = end-start;
  var textElem = document.createTextNode(subtractTime + 'ms  ');
  document.getElementById('subtractResult').appendChild(textElem);
  completed();
}

function testMultiply()
{	
  var value1=123;
  var value2=234;
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = value2 * value1;
  }
  end = new Date();
  multiplyTime = end-start;
  var textElem = document.createTextNode(multiplyTime + 'ms  ');
  document.getElementById('multiplyResult').appendChild(textElem);
  completed();
}

function testDivide()
{
  var value1=123;
  var value2=234;
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = value2 / value1;
  }
  end = new Date();
  divideTime = end-start;
  var textElem = document.createTextNode(divideTime + 'ms  ');
  document.getElementById('divideResult').appendChild(textElem);
  completed();
}

function testDivide2()
{
  var value1=123;
  var value2=234;
  var value3;
  start = new Date();
  var temp= (1 / value1);
  for (var i=0; i<=1000000; i++)
  {
    value3 = value2 * temp;
  }
  end = new Date();
  divide2Time = end-start;
  var textElem = document.createTextNode(divide2Time + 'ms  ');
  document.getElementById('divide2Result').appendChild(textElem);
  completed();
}



function testFromArray()
{
  var value3;
  var value4 = new Array(1,2,3,4,5,6,7,8,9,10);
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = value4[6];
  }
  end = new Date();
  fromArrayTime = end-start;
  var textElem = document.createTextNode(fromArrayTime + 'ms  ');
  document.getElementById('fromArrayResult').appendChild(textElem);
  completed();
}

function testParseInt()
{
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = parseInt(123.23);
  }
  end = new Date();
  parseIntTime = end-start;
  var textElem = document.createTextNode(parseIntTime + 'ms  ');
  document.getElementById('parseIntResult').appendChild(textElem);
  completed();
}

function testVar()
{
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    var a=1; var b=2; var c=3;
  }
  end = new Date();
  varTime = end-start;
  var textElem = document.createTextNode(varTime + 'ms  ');
  document.getElementById('varResult').appendChild(textElem);
  completed();
  
}

function testSin()
{
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = Math.sin(.1);
  }
  end = new Date();
  sinTime = end-start;
  var textElem = document.createTextNode(sinTime + 'ms  ');
  document.getElementById('sinResult').appendChild(textElem);
  completed();			
}

function testFloor()
{
  var value3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value3 = Math.floor(43.341);
  }
  end = new Date();
  floorTime = end-start;
  var textElem = document.createTextNode(floorTime + 'ms  ');
  document.getElementById('floorResult').appendChild(textElem);
  completed();			
}

function testIf()
{
  var value1=123;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    if (value1<12)
    {
      void(0);
    }
  }
  end = new Date();
  ifTime = end-start;
  var textElem = document.createTextNode(ifTime + 'ms  ');
  document.getElementById('ifResult').appendChild(textElem);
  completed();			
}

function testReadGlobal()
{
  var value;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    value = gValue;
  }
  end = new Date();
  readGlobalTime = end-start;
  var textElem = document.createTextNode(readGlobalTime + 'ms  ');
  document.getElementById('readGlobalResult').appendChild(textElem);
  completed();			
}

function testConcatStrings()
{
  var string1 = 'abcdefghijklmnopqrstuvxyz';
  var string2 = 'abcdefghijklmnopqrstuvxyz';
  var string3;
  start = new Date();
  for (var i=0; i<=1000000; i++)
  {
    string3 = string1 + string2;
  }
  end = new Date();
  concatStringsTime = end-start;
  var textElem = document.createTextNode(concatStringsTime + 'ms  ');
  document.getElementById('concatStringsResult').appendChild(textElem);
  completed();			
}

function testSortArray()
{
  var theArray = new Array;
  for(i=0; i<=10000; i++)
    theArray[i] = parseInt(Math.random()*10000);
  start = new Date();
  theArray.sort();
  end = new Date();
  sortArrayTime = end-start;
  var textElem = document.createTextNode(sortArrayTime + 'ms  ');
  document.getElementById('sortArrayResult').appendChild(textElem);
  completed();			
}


testFor();
testAdd();
testSubtract();
testMultiply();
testDivide();
testDivide2();
testFromArray();
testParseInt();
testVar();
testSin();
testFloor();
testIf();
testReadGlobal();
testConcatStrings();
testSortArray();

