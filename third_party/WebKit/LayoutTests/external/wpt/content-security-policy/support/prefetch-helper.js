test(t => {
  assert_true(document.createElement('link').relList.supports('prefetch'));
}, "Browser supports prefetch.");

test(t => {
  assert_true(!!window.PerformanceResourceTiming);
}, "Browser supports performance APIs.");

async function waitUntilResourceDownloaded(url) {
  await new Promise((resolve, reject) => {
    if (performance.getEntriesByName(url).length >= 1)
      resolve();

    let observer = new PerformanceObserver(list => {
      list.getEntries().forEach(entry => {
        if (entry.name == url) {
          resolve();
        }
      });
    });
  });
}

async function assert_resource_not_downloaded(test, url) {
  if (performance.getEntriesByName(url).length >= 1) {
    (test.unreached_func(`'${url}' should not have downloaded.`))();
  } else {

  }
}

function assert_link_prefetches(test, link) {
  assert_no_csp_event_for_url(test, link.href);

  link.onerror = test.unreached_func('onerror should not fire.');
  link.onload = test.step_func(_ => {
    waitUntilResourceDownloaded(link.href).then(test.step_func_done());
  });

  document.head.appendChild(link);
}

function assert_link_does_not_prefetch(test, link) {
  let cspEvent = false;
  let errorEvent = false;

  waitUntilCSPEventForURL(test, link.href)
      .then(test.step_func(e => {
        cspEvent = true;
        assert_equals(e.violatedDirective, "prefetch-src");
        assert_equals(e.effectiveDirective, "prefetch-src");

        if (errorEvent)
          test.done();
      }));

  link.onerror = test.step_func(e => {
    errorEvent = true;
    if (cspEvent)
      test.done();
  });
  link.onload = test.unreached_func('onerror should not fire.');

  document.head.appendChild(link);
}
