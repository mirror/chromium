<?php
    header("Content-Security-Policy: script-src 'nonce-abc'", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.js?without-nonce>;rel=preload;as=script", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.js?with-nonce>;rel=preload;as=script;nonce=abc", false);
?>
<!DOCTYPE html>
<html>
<head></head>
<body>
<script nonce="abc" src="/resources/testharness.js"></script>
<script nonce="abc" src="/resources/testharnessreport.js"></script>
<script nonce="abc">
    var t = async_test('Makes sure that Link headers preload resources with CSP nonce');
</script>
<script nonce="abc" src="/resources/slow-script.pl?delay=200"></script>
<script nonce="abc">
    window.addEventListener('load', t.step_func(function() {
        var url = 'http://127.0.0.1:8000/resources/dummy.js';
        assert_equals(performance.getEntriesByName(url + '?without-nonce').length, 0);
        assert_equals(performance.getEntriesByName(url + '?with-nonce').length, 1);
        t.done();
    }));
</script>
</body>
</html>
