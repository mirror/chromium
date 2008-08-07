// This is okay.
function foo() { const a; var a; }

// This isn't.
(function () {
  const a;
  var a;
})();
