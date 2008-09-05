// This is okay.
function foo() { var a; const a; }

// This isn't.
(function () {
  var a;
  const a;
})();
