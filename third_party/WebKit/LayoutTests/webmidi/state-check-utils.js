function checkStateTransition(options) {
    debug("Check state transition for " + options.method + " on " +
          options.initialconnection + " state.");
    debug("- check initial state.");
    window.port = options.port;
    shouldBeEqualToString("port.connection", options.initialconnection);
    var checkHandler = function(e) {
        window.eventport = e.port;
        testPassed("handler is called with port " + eventport + ".");
        if (options.initialconnection == options.finalconnection) {
            testFailed("onstatechange handler should not be called here.");
        }
        shouldBeEqualToString("eventport.id", options.port.id);
        shouldBeEqualToString("eventport.connection", options.finalconnection);
    };
    var listener_option = {
        resolver: null,
        wait_count: 0,
        check: function() {
            if (!this.resolver)
                return;
            if (--this.wait_count == 0) {
                this.resolver();
                this.resolver = null;
            }
        }
    };
    port.onstatechange = function(e) {
        debug("- check port handler.");
        checkHandler(e);
        listener_option.check();
    };
    access.onstatechange = function(e) {
        debug("- check access handler.");
        checkHandler(e);
        listener_option.check();
    };
    if (options.method == "setonmidimessage") {
        return new Promise(function(success, error) {
            listener_option.resolver = success;
            listener_option.wait_count = 2;
            port.onmidimessage = function() {};
        });
    }
    if (options.method == "addeventlistener") {
        return new Promise(function(success, error) {
            listener_option.resolver = success;
            listener_option.wait_count = 2;
            port.addEventListener("midimessage", function() {});
        });
    }
    if (options.method == "send") {
        return new Promise(function(success, error) {
            listener_option.resolver = success;
            listener_option.wait_count = 2;
            port.send([]);
        });
    }
    // |method| is expected to be "open" or "close".
    return port[options.method]().then(function(p) {
        window.callbackport = p;
        debug("- check callback arguments.");
        testPassed("callback is called with port " + callbackport + ".");
        shouldBeEqualToString("callbackport.id", options.port.id);
        shouldBeEqualToString("callbackport.connection", options.finalconnection);
        debug("- check final state.");
        shouldBeEqualToString("port.connection", options.finalconnection);
    }, function(e) {
        testFailed("error callback should not be called here.");
        throw e;
    });
}


