window.onload = function() {
  test(function() {
    assert_equals(undefined, window.didExecuteInlineParsingBlockingScript);
  }, 'inline parser blocking script should be blocked');

  test(function() {
    assert_true(window.didExecuteInlineAsyncScript);
  }, 'inline async script should not be blocked');

  test(function() {
    assert_equals(undefined, window.didExecuteExternalParsingBlockingScript);
  }, 'external parser blocking script should be blocked');

  test(function() {
    assert_true(window.didExecuteExternalAsyncScript);
  }, 'external async script should not be blocked');

  test(function() {
    assert_true(window.didExecuteExternalDeferredScript);
  }, 'external defer script should not be blocked');
};
