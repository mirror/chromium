<?php
    header("Link: <http://127.0.0.1:8000/resources/square.png>;rel=preload;as=image", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.js>;rel=preload;as=script", false);
    header("Link: <http://127.0.0.1:8000/resources/dummy.css>;rel=preload;as=style", false);
?>
<!DOCTYPE html>
<html>
<head></head>
<body>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script>
    var t = async_test('Makes sure that Link headers preload resources');
</script>
<script src="/resources/slow-script.pl?delay=200"></script>
<script>
    window.addEventListener("load", t.step_func(function() {
        var entries = performance.getEntriesByType("resource");
        assert_equals(entries.length, 7);
        assert_equals(entries[0].name, "http://127.0.0.1:8000/resources/dummy.js");
        assert_equals(entries[1].name, "http://127.0.0.1:8000/resources/dummy.css");
        assert_equals(entries[2].name, "http://127.0.0.1:8000/resources/testharness.js");
        assert_equals(entries[3].name, "http://127.0.0.1:8000/resources/testharnessreport.js");
        assert_equals(entries[4].name, "http://127.0.0.1:8000/resources/slow-script.pl?delay=200");
        assert_equals(entries[5].name, "http://127.0.0.1:8000/resources/square.png");
        assert_equals(entries[6].name, "http://127.0.0.1:8000/resources/square.png?dynamic");
        t.done();
    }));
    var img = new Image();
    img.src = "http://127.0.0.1:8000/resources/square.png?dynamic";
</script>
</body>
</html>
