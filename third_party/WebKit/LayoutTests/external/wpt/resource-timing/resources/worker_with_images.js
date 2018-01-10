let numComplete = 0;

function checkDone() {
  ++numComplete;
  if (numComplete == 2) {
    postMessage('');
  }
}

function makeRequest(request) {
  var xhr = new XMLHttpRequest;
  xhr.open('get', request, true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4) {
      checkDone();
    }
  }
  xhr.send();
}
makeRequest('blue.png?type=image&id=1');
makeRequest('resource_timing_test0.png?type=image&id=2');
