window.addEventListener("securitypolicyviolation", function(e) {
  window.top.postMessage(e.violatedDirective, "*");
});