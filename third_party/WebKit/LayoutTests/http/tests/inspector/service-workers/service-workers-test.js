var initialize_ServiceWorkersTest = function() {
    InspectorTest.preloadModule('application_test_runner');
}

function loadIframe(url, frame_id) {
  let callback;
  let promise = new Promise((fulfill) => callback = fulfill);
  let frame = document.createElement('iframe');
  if (frame_id !== undefined) {
    frame.id = frame_id;
  }
  frame.src = url;
  frame.onload = callback;
  document.body.appendChild(frame);
  return promise;
}

function unloadIframe(frame_id) {
  document.getElementById(frame_id).remove();
}

function waitForActivated(worker) {
  if (worker.state === 'activated')
    return Promise.resolve();
  if (worker.state === 'redundant')
    return Promise.reject(new Error('The worker is redundant'));
  return new Promise(resolve => {
      worker.addEventListener('statechange', () => {
          if (worker.state === 'activated')
            resolve();
        });
    });
}
