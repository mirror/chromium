var s = "a";
for (var i = 0; i < 17; i++)
    s += s;

assertThrows('new RegExp(s);');

assertThrows('/(([ab]){30}){3360}/');
assertThrows('/(([ab]){30}){0,3360}/');
assertThrows('/(([ab]){30}){10,3360}/');
assertThrows('/(([ab]){0,30}){3360}/');
assertThrows('/(([ab]){0,30}){0,3360}/');
assertThrows('/(([ab]){0,30}){10,3360}/');
assertThrows('/(([ab]){10,30}){3360}/');
assertThrows('/(([ab]){10,30}){0,3360}/');
assertThrows('/(([ab]){10,30}){10,3360}/');
assertThrows('/(([ab]){12})(([ab]){65535}){1680}(([ab]){38}){722}([ab]){27}/');
