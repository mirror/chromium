function delete_then_open(db_name, upgrade_func, body_func) {
  let delete_request = indexedDB.deleteDatabase(db_name);
  delete_request.onerror = PerfTestRunner.logFatalError.bind('deleteDatabase should not fail');
  delete_request.onsuccess = function(e) {
    let open_request = indexedDB.open(db_name);
    open_request.onupgradeneeded = function() {
      upgrade_func(open_request.result, open_request);
    }
    open_request.onsuccess = function() {
      body_func(open_request.result, open_request);
    }
    open_request.onerror = function(e) {
      window.PerfTestRunner.logFatalError("Error setting up database " + db_name + ". Error: " + e.type);
    }
  }
}

function create_incremental_barrier(callback) {
  let count = 0;
  let called = false;
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
  return new Promise((resolve, reject) => {
    txn.oncomplete = resolve;
    txn.onabort = reject;
  });
}

function report_done() {
  window.parent.postMessage({
    message: "done"
  }, "*");
}

function report_error(event) {
  console.log(event);
  window.parent.postMessage({
    message: "error",
    data: event
  }, "*", );
}

if (window.PerfTestRunner) {
  window.PerfTestRunner.measurePageLoadTimeAfterDoneMessage = function(test) {

    let is_done = false;
    let outer_done = test.done;
    test.done = function(done) {
      is_done = true;
      if (outer_done)
        done();
    }

    test.run = function() {
      let file = PerfTestRunner.loadFile(test.path);

      let run_once = function(run_finished_callback) {
        let startTime;

        PerfTestRunner.logInfo("Testing " + file.length + " byte document.");

        let iframe = document.createElement("iframe");
        test.iframe = iframe;
        document.body.appendChild(iframe);

        iframe.sandbox = '';
        // Prevent external loads which could cause write() to return before completing the parse.
        iframe.style.width = "600px";
        // Have a reasonable size so we're not line-breaking on every character.
        iframe.style.height = "800px";
        iframe.contentDocument.open();

        let event_handler = (event)=>{
          if (event.data.message == undefined) {
            console.log("Unknown message: ", event);
          } else if (event.data.message == "done") {
            PerfTestRunner.measureValueAsync(PerfTestRunner.now() - startTime);
            PerfTestRunner.addRunTestEndMarker();
            document.body.removeChild(test.iframe);
            run_finished_callback();
          } else if (event.data.message == "error") {
            console.log("Error in page", event.data.data);
            PerfTestRunner.logFatalError("error in page: " + event.data.data.type);
          } else {
            console.log("Unknown message: ", event);
          }
          window.removeEventListener("message", event_handler);
        }
        window.addEventListener("message", event_handler, false);

        PerfTestRunner.addRunTestStartMarker();
        startTime = PerfTestRunner.now();

        iframe.contentDocument.write(file);
        PerfTestRunner.forceLayout(iframe.contentDocument);

        iframe.contentDocument.close();
      }

      let iteration_callback = function() {
        if (!is_done)
          run_once(iteration_callback);
      }

      run_once(iteration_callback);
    }

    PerfTestRunner.startMeasureValuesAsync(test)
  }
}
