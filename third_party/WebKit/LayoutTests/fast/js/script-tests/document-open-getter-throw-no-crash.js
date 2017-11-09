description("document.open() - propagate no exception when accessing window.open.");

var frame = document.body.appendChild(document.createElement("iframe"));
frame.contentWindow.__defineGetter__("open", function() { throw 'FAIL: Reached unreachable code';});
frame.contentDocument.open(1, 1, 1, 1, 1);

