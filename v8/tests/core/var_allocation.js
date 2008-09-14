// This piece of code caused assertion failures during ScopeInfo
// construction when --usage_computation is enabled. The problem
// was that both x and y end up context-allocated, with y having a
// higher use-count the x. As a result, y appears first in the use-count
// sorted locals list, but x is allocated first anyway because it's a parameter.
 
function g(x) {
  var y;
  function() {
    y = h(x);
  }
}
