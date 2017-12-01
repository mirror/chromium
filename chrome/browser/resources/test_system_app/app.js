'use strict';

var browserProxy;

(function() {

function initialize() {
  browserProxy = new mojom.TestSystemAppPtr;
  Mojo.bindInterface(
      mojom.TestSystemApp.name,
      mojo.makeRequest(browserProxy).handle);
  browserProxy.getNumber().then((result) =>
      document.getElementById('answer').innerHTML = result.number);
}

document.addEventListener('DOMContentLoaded', initialize);

})();
