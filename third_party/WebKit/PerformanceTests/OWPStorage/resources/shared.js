function delete_then_open(db_name, upgrade_func, body_func) {
  var delete_request = indexedDB.deleteDatabase(db_name);
  delete_request.onerror = PerfTestRunner.logFatalError.bind('deleteDatabase should not fail');
  delete_request.onsuccess = function(e) {
    var open_request = indexedDB.open(db_name);
    open_request.onupgradeneeded = function() {
      upgrade_func(open_request.result, open_request);
    }
    open_request.onsuccess = function() {
      body_func(open_request.result, open_request);
    }
  }
}

function create_incremental_barrier(callback) {
  var count = 0;
  var called = false;
  return function() {
    if (called)
      PerfTestRunner.logFatalError("Barrier already used.");
    ++count;
    return function() {
      --count;
      if (count === 0) {
        if (called)
          PerfTestRunner.logFatalError("Barrier already used.");
        called = true;
        callback();
      }
    }
  }
}

function transaction_complete_promise(txn) {
  return new Promise(function(resolve, reject) {
    txn.oncomplete = resolve;
    txn.onerror = reject;
  }
  );
}

function report_done() {
  window.parent.postMessage("done", "*");
}

function report_error(event) {
  window.parent.postMessage(event, "*", event);
}

if (window.PerfTestRunner) {
  window.PerfTestRunner.measurePageLoadTimeAfterDoneMessage = function(test) {
    var file = PerfTestRunner.loadFile(test.path);
    test.run = async function() {
      if (!test.chunkSize)
        this.chunkSize = 50000;

      var chunks = [];
      // The smaller the chunks the more style resolves we do.
      // Smaller chunk sizes will show more samples in style resolution.
      // Larger chunk sizes will show more samples in line layout.
      // Smaller chunk sizes run slower overall, as the per-chunk overhead is high.
      var chunkCount = Math.ceil(file.length / this.chunkSize);
      for (var chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {
        var chunk = file.substr(chunkIndex * this.chunkSize, this.chunkSize);
        chunks.push(chunk);
      }

      PerfTestRunner.logInfo("Testing " + file.length + " byte document in " + chunkCount + " " + this.chunkSize + " byte chunks.");

      var iframe = document.createElement("iframe");
      document.body.appendChild(iframe);

      iframe.sandbox = '';
      // Prevent external loads which could cause write() to return before completing the parse.
      iframe.style.width = "600px";
      // Have a reasonable size so we're not line-breaking on every character.
      iframe.style.height = "800px";
      iframe.contentDocument.open();

      var message_promise = new Promise((resolve,reject)=>{
        var event_handler = (event)=>{
          console.log("got event", event);
          if (event.data == "done") {
            resolve();
          } else if (event.data == "error") {
            reject();
          } else {
            console.log("Unknown message: ", event);
          }
          window.removeEventListener("message", event_handler);
        }

        window.addEventListener("message", event_handler, false);
      }
      );

      for (var chunkIndex = 0; chunkIndex < chunks.length; chunkIndex++) {
        iframe.contentDocument.write(chunks[chunkIndex]);
        PerfTestRunner.forceLayout(iframe.contentDocument);
      }

      await message_promise;
      PerfTestRunner.forceLayout(iframe.contentDocument);

      iframe.contentDocument.close();
      document.body.removeChild(iframe);
    }
    ;

    PerfTestRunner.measureTime(test);
  }
}
