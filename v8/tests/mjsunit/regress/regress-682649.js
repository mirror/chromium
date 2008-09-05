// ecma/Expressions/11.1.1.js
// Should return [object global], v8 returns [object Object]


var GLOBAL_OBJECT = this.toString();

assertEquals(GLOBAL_OBJECT, eval("this.toString()"));
