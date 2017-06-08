<?php
header("Suborigin: foobar 'unsafe-credentials'");
setcookie("TestCookieSuboriginUnsafeCredentialsSamePO", "cookieisset", 0, "/");
?>
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>'unsafe-credentials' forces credentials for same physical origin only</title>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
</head>
<body>
<div id="container"></div>
<script>
const same_physical_origin_url_params = new URLSearchParams();
for (const params of [
  ['resultvar', 'result1'],
  ['cookie', 'TestCookieSuboriginUnsafeCredentialsSamePO'],
  ['ACACredentials', 'true'],
  ['ACAOrigin', 'http-so://foobar.127.0.0.1:8000'],
]) {
  same_physical_origin_url_params.append(params[0], params[1]);
}
const same_physical_origin_url = '/security/resources/cors-script.php?' +
  same_physical_origin_url_params.toString();

const diff_physical_origin_url_params = new URLSearchParams();
for (const params of [
  ['resultvar', 'result2'],
  ['cookie', 'TestCookieSuboriginUnsafeCredentialsDiffPO'],
  ['ACACredentials', 'true'],
  ['ACAOrigin', 'http-so://foobar.127.0.0.1:8000'],
]) {
  diff_physical_origin_url_params.append(params[0], params[1]);
}
const diff_physical_origin_url =
  'http://localhost:8000/security/resources/cors-script.php?' +
  diff_physical_origin_url_params.toString();

const redirect_to_same_physical_origin_url_params = new URLSearchParams();
for (const params of [
  ['url', same_physical_origin_url],
  ['expect_cookie', 'true'],
  ['cookie_name', 'TestCookieSuboriginUnsafeCredentialsSamePO'],
  ['ACAOrigin', 'http-so://foobar.127.0.0.1:8000'],
  ['ACASuborigin', 'foobar'],
  ['ACACredentials', 'true'],
]) {
  redirect_to_same_physical_origin_url_params.append(params[0], params[1]);
}
const same_physical_origin_with_redirect_url =
  '/security/suborigins/resources/redirect.php?' +
  redirect_to_same_physical_origin_url_params.toString();

var cross_origin_iframe_loaded_resolver = undefined;
var cross_origin_iframe_loaded = new Promise(function(resolve) {
    cross_origin_iframe_loaded_resolver = resolve;
});

// Waits for a message from the iframe so it knows the cross-origin iframe
// cookie is set.
window.addEventListener('message', function() {
    cross_origin_iframe_loaded_resolver();
  });

var iframe = document.createElement('iframe');
iframe.src =
  'http://localhost:8000/security/suborigins/resources/' +
  'set-cookie-and-postmessage-parent.php?' +
  'name=TestCookieSuboriginUnsafeCredentialsDiffPO&' +
  'value=cookieisset';
document.body.appendChild(iframe);

function xhr_test(url, result_name, expected_value) {
  return function(test) {
      var xhr = new XMLHttpRequest();
      xhr.onload = test.step_func(_ => {
          assert_equals(
            xhr.responseText, result_name + ' = "' + expected_value + '";');
          test.done();
        });
      xhr.onerror = test.unreached_func();

      xhr.open('GET', url);
      xhr.send();
    };
}

async_test(
  xhr_test(same_physical_origin_url, 'result1', 'cookieisset'),
  '\'unsafe-credentials\' forces credentials with XHR to same ' +
  'physical origin');

async_test(
  xhr_test(same_physical_origin_with_redirect_url, 'result1', 'cookieisset'),
  '\'unsafe-credentials\' forces credentials across redirect with XHR to ' +
  'same physical origin');

async_test(
  xhr_test(diff_physical_origin_url, 'result2', ''),
  '\'unsafe-credentials\' does not force credentials with XHR ' +
  'to different physical origin');

function fetch_test(url, result_name, expected_value) {
  return function(test) {
    return fetch(url, { credentials: 'same-origin' })
      .then(r => {
        return r.text();
      })
      .then(text => {
        assert_equals(text, result_name + ' = "' + expected_value + '";');
      });
  };
}

promise_test(
  fetch_test(same_physical_origin_url, 'result1', 'cookieisset'),
  '\'unsafe-credentials\' forces credentials with same-origin ' +
  'Fetch to same physical origin');

promise_test(
  fetch_test(same_physical_origin_with_redirect_url, 'result1', 'cookieisset'),
  '\'unsafe-credentials\' forces credentials across redirect with ' +
  'same-origin Fetch to same physical origin');

promise_test(
  fetch_test(diff_physical_origin_url, 'result2', ''),
  '\'unsafe-credentials\' does not force credentials with ' +
  'same-origin Fetch to different physical origin');

function script_element_test(url, result_name, expected_value) {
  return function(test) {
      window[result_name] = false;
      var script = document.createElement('script');
      script.setAttribute('crossOrigin', 'anonymous');
      script.src = url;
      script.onload = test.step_func(function() {
          assert_equals(window[result_name], expected_value);
          test.done();
        });
      document.body.appendChild(script);
    };
}

async_test(
  script_element_test(same_physical_origin_url, 'result1', 'cookieisset'),
  '\'unsafe-credentials\' forces credentials with ' +
  'crossorigin="anonymous" script tag to same physical origin');

async_test(
  script_element_test(same_physical_origin_with_redirect_url,
    'result1', 'cookieisset'),
  '\'unsafe-credentials\' forces credentials across redirect with ' +
  'crossorigin="anonymous" script tag to same physical origin');

async_test(
  script_element_test(diff_physical_origin_url, 'result2', ''),
  '\'unsafe-credentials\' does not force credentials with ' +
  'crossorigin="anonymous" script tag to different physical origin');
</script>
</body>
</html>
