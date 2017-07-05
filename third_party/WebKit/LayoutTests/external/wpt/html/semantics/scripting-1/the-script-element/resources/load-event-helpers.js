"use strict";
function onLoad(t) {
    t.test.step(function() {
        if (t.load_event_to_be_fired) {
            assert_true(t.executed,
                'Load event should be fired after script execution');
            // Delay done() a little to ensure assert_unreached() is not
            // executed just after this load event.
            setTimeout(t.test.step_func_done(), 100);
        } else {
            assert_unreached('Load event should not be fired.');
        }
    });
};
function onError(t) {
    t.test.step(function() {
        if (t.error_event_to_be_fired) {
            assert_false(t.executed);
            // Delay done() a little to ensure assert_unreached() is not
            // executed just after this error event.
            setTimeout(t.test.step_func_done(), 100);
        } else {
            assert_unreached('Error event should not be fired.');
        }
    });
};

function event_test(name, load_to_be_fired, error_to_be_fired) {
  return {
      test: async_test(name),
      executed: false,
      load_event_to_be_fired: load_to_be_fired,
      error_event_to_be_fired: error_to_be_fired
  };
}

function onExecute(t) {
    t.executed = true;
    // Delay done() a little to ensure assert_unreached() is not
    // executed just after this.
    setTimeout(t.test.step_func_done(), 100);
}
