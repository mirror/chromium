// Spidermonkey and IE has some string functions useable for building
// HTML.
function CheckSimple(f, tag) {
  assertEquals('<' + tag + '>foo</' + tag + '>',
               "foo"[f]().toLowerCase()); 
};
var simple = { big: 'big', blink: 'blink', bold: 'b',
               fixed: 'tt', italics: 'i', small: 'small',
               strike: 'strike', sub: 'sub', sup: 'sup' };
for (var i in simple) CheckSimple(i, simple[i]);


function CheckCompound(f, tag, att) {
  assertEquals('<' + tag + ' ' + att + '="bar">foo</' + tag + '>',
               "foo"[f]("bar").toLowerCase());
};
CheckCompound('anchor', 'a', 'name');
CheckCompound('link', 'a', 'href');
CheckCompound('fontcolor', 'font', 'color');
CheckCompound('fontsize', 'font', 'size');
