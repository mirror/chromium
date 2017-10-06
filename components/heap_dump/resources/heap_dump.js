

function onLoad() {
  // Register to receive a stream of event notifications.
  chrome.send("registerForEvents");
}

function getAllProcessesCallback(processes) {
  processes.forEach(function (process) {
    chrome.send("requestProcessHeapDump", [process.id]);
  });
}

document.addEventListener(
    'DOMContentLoaded', onLoad, false);
