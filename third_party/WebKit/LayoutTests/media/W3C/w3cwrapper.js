function getVideoURI() {
  return "../../../content/test-vp9-opus.webm";
}

function getAudioURI() {
  return "../../../content/silence-opus.webm";
}

function testStep(testFunction) {
  try {
    testFunction();
  } catch (e) {
    testFailed('Aborted with exception: ' + e.message);
  }
}

function testDone() {
  // Match the semantics of testharness.js done(), where nothing that
  // happens after that call has any effect on the test result.
  if (!window.wasFinishJSTestCalled) {
    finishJSTest();
    assert_equals = assert_true = assert_false = function() { };
  }
}

function test(testFunction) {
  description(document.title);
  testStep(testFunction);
}

function async_test(title, options) {
  window.jsTestIsAsync = true;
  description(title);
  return {
    step: testStep,
    done: testDone
  }
}

document.write("<p id=description></p><div id=console></div>");
document.write("<script src='../../../../resources/js-test.js'></" + "script>");

assert_equals = function(a, b) { shouldBe('"' + a + '"', '"' + b + '"'); }
assert_true = function(a) { shouldBeTrue("" + a); }
assert_false = function(a) { shouldBeFalse("" + a); }

if (window.testRunner)
  testRunner.dumpAsText();
