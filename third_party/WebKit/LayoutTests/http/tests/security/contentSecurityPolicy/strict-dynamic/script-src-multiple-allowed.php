<?php
    header("Content-Security-Policy: script-src 'nonce-abcdefg' 'strict-dynamic', script-src * 'unsafe-inline'");
?>
<!DOCTYPE html>
<html>
<head>
    <script src="/resources/testharness.js" nonce="abcdefg"></script>
    <script src="/resources/testharnessreport.js" nonce="abcdefg"></script>
</head>
<body>
    <script nonce="abcdefg">
        function generateURL(type) {
          return 'http://localhost:8000/security/contentSecurityPolicy/resources/loaded.js?' + type;
        }

        var loaded = {};
        var blocked = {};
        window.addEventListener("message", function (e) {
          loaded[e.data] = true;
        });
        document.addEventListener("securitypolicyviolation", function (e) {
          blocked[e.lineNumber] = true;
        });
    </script>
    <!-- Need to individually wrap test cases in script blocks. Violation reports triggered by document.write() calls while the parser is waiting on blocking scipts are missing line numbers. See: https://crbug.com/649085. -->
    <script nonce="abcdefg">
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("append");
          e.onload = t.step_func(function () {
              // Delay the check until after the postMessage has a chance to execute.
              setTimeout(t.step_func_done(function () {
                assert_true(loaded[generateURL("append")]);
              }), 1);
          });
          e.onerror = t.unreached_func("Error should not be triggered.");
          document.body.appendChild(e);
        }, "Script injected via 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("append-async");
          e.async = true;
          e.onload = t.step_func(function () {
              // Delay the check until after the postMessage has a chance to execute.
              setTimeout(t.step_func_done(function () {
                assert_true(loaded[generateURL("append-async")]);
              }), 1);
          });
          e.onerror = t.unreached_func("Error should not be triggered.");
          document.body.appendChild(e);
        }, "Async script injected via 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("append-defer");
          e.defer = true;
          e.onload = t.step_func(function () {
              // Delay the check until after the postMessage has a chance to execute.
              setTimeout(t.step_func_done(function () {
                assert_true(loaded[generateURL("append-defer")]);
              }), 1);
          });
          e.onerror = t.unreached_func("Error should not be triggered.");
          document.body.appendChild(e);
        }, "Deferred script injected via 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          document.write("<scr" + "ipt src='" + generateURL("write") + "'></scr" + "ipt>");
          setTimeout(t.step_func_done(function () {
            assert_equals(loaded[generateURL("write")], undefined);
            assert_true(blocked[69]);
          }), 1);
        }, "Script injected via 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          document.write("<scr" + "ipt defer src='" + generateURL("write-defer") + "'></scr" + "ipt>");
          setTimeout(t.step_func_done(function () {
            assert_equals(loaded[generateURL("write-defer")], undefined);
            assert_true(blocked[78]);
          }), 1);
        }, "Deferred script injected via 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg">
        async_test(function (t) {
          document.write("<scr" + "ipt async src='" + generateURL("write-async") + "'></scr" + "ipt>");
          setTimeout(t.step_func_done(function () {
            assert_equals(loaded[generateURL("write-async")], undefined);
            assert_true(blocked[87]);
          }), 1);
        }, "Async script injected via 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("defer-append");
          e.onload = t.step_func(function () {
              // Delay the check until after the postMessage has a chance to execute.
              setTimeout(t.step_func_done(function () {
                assert_true(loaded[generateURL("defer-append")]);
                assert_equals(blocked[generateURL("defer-append")], undefined);
              }), 1);
          });
          e.onerror = t.unreached_func("Error should not be triggered.");
          document.body.appendChild(e);
        }, "Script injected via deferred 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("defer-append-async");
          e.async = true;
          e.onload = t.step_func(function () {
              // Delay the check until after the postMessage has a chance to execute.
              setTimeout(t.step_func_done(function () {
                assert_true(loaded[generateURL("defer-append-async")]);
                assert_equals(blocked[generateURL("defer-append-async")], undefined);
              }), 1);
          });
          e.onerror = t.unreached_func("Error should not be triggered.");
          document.body.appendChild(e);
        }, "Async script injected via deferred 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          var e = document.createElement('script');
          e.src = generateURL("defer-append-defer");
          e.defer = true;
          e.onload = t.step_func(function () {
              // Delay the check until after the postMessage has a chance to execute.
              setTimeout(t.step_func_done(function () {
                assert_true(loaded[generateURL("defer-append-defer")]);
                assert_equals(blocked[generateURL("defer-append-defer")], undefined);
              }), 1);
          });
          e.onerror = t.unreached_func("Error should not be triggered.");
          document.body.appendChild(e);
        }, "Deferred script injected via deferred 'appendChild' is allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          document.write("<scr" + "ipt src='" + generateURL("defer-write") + "'></scr" + "ipt>");
          setTimeout(t.step_func_done(function () {
            assert_equals(loaded[generateURL("defer-write")], undefined);
            assert_true(blocked[143]);
          }), 1);
        }, "Script injected via deferred 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          document.write("<scr" + "ipt defer src='" + generateURL("defer-write-defer") + "'></scr" + "ipt>");
          setTimeout(t.step_func_done(function () {
            assert_equals(loaded[generateURL("defer-write-defer")], undefined);
            assert_true(blocked[152]);
          }), 1);
        }, "Deferred script injected via deferred 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
    <script nonce="abcdefg" defer>
        async_test(function (t) {
          document.write("<scr" + "ipt async src='" + generateURL("defer-write-async") + "'></scr" + "ipt>");
          setTimeout(t.step_func_done(function () {
            assert_equals(loaded[generateURL("defer-write-async")], undefined);
            assert_true(blocked[161]);
          }), 1);
        }, "Async script injected via deferred 'document.write' is not allowed with 'strict-dynamic'.");
    </script>
</body>
</html>
