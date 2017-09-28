'use strict';

// See https://github.com/whatwg/html/pull/3011#issuecomment-331187136
// and https://www.chromestatus.com/feature/6170540112871424
const kMetaHttpEquivSetCookieIsGone = true;

const kHasDocument = typeof document !== 'undefined';

const kTestTitle = decodeURIComponent(
    ((location.hash || '#').match(/(^#|&)title=([^&]*)/) || [])[2] ||
      (kHasDocument ? encodeURIComponent(document.title) :
       location.pathname.replace(/^.*\/([.]+)(\.[^\/]*)?$/, '$1')));

if (kHasDocument) document.title = kTestTitle;

if (self.testRunner) testRunner.setBlockThirdPartyCookies(false);

const redirect = () => {
  promise_test(async () => {
    const testNameParts =
        location.pathname.replace(
            /.*\//,
            ''
        ).split(
            '.',
            1
        )[0].split('_');
    const kIsStatic = testNameParts[testNameParts.length - 1] === 'static';
    if (kIsStatic) testNameParts.pop();
    const testName =
          testNameParts.join('_').replace(/_([^_])/gi, x => x[1].toUpperCase());
    let hash = (location.hash || '#');
    if (testName.match(/^test\w+$/)) {
      hash += (hash === '#' ? '' : '&').replace(
          /(^#|&)test=[^&]*(&test=[^&]*)*&?/g,
          '$1') + 'test=' + encodeURIComponent(testName);
    }
    if (kIsStatic) hash += (hash === '#' ? '' : '&').replace(
        /(^#|&)static=[^&]*(&static=[^&]*)*&?/g,
        '$1') + 'static=true';
    hash += (hash === '#' ? '' : '&').replace(
          /(^#|&)title=[^&]*(&title=[^&]*)*&?/g,
        '$1') + 'title=' + encodeURIComponent(kTestTitle);
    if (location.hash != hash) location.hash = hash;
    let path = location.pathname.replace(
        /\/[^\/.]*(\.[^\/]+)$/,
        '/cookie_store_tests$1').split('.https.html').join('.html');
    assert_not_equals(path, location.pathname, 'failed to change path');
    location.pathname = path;
    await new Promise(ignoredResolve => {});
  }, kTestTitle + ' redirect (never resolves!)');
};

const includeTest = (test, opt_excludeFromAll) => {
  const testName = test.name;
  assert_equals(!!testName.match(/^test\w+/), true, 'includeTest: ' + testName);
  assert_equals(typeof eval(testName), 'function', 'includeTest: ' + testName);
  let testParams =
        (location.hash || '#').substr(1).split('&').filter(
            x => x.match(/^test=/)).map(x => decodeURIComponent(x));
  if (!testParams.length) testParams = ['test=all'];
  const filterSet =
        testParams.map(x => x.split('=', 2)[1]).join(',').split(',').reduce(
            (set, name) => Object.assign(set, {[name]: true}), {});
  for (let name in filterSet) {
    if (name === 'all' || !filterSet.hasOwnProperty(name)) continue;
    assert_equals(!!name.match(/^test\w+/), true, '#test=' + testName);
    assert_equals(typeof eval(name), 'function', '#test=' + testName);
  }
  return (filterSet.all && !opt_excludeFromAll) ||
      filterSet.hasOwnProperty(testName) && filterSet[testName];
}

const kIsUnsecured = location.protocol !== 'https:';

const kIsStatic = !!(location.hash || '#').match(/(^#|&)static=true(&|$)/);

const kCookieHelperCgi = 'resources/cookie_helper.py';

const promise_rejects_when_unsecured = async (
    testCase,
    code,
    promise,
    message = 'Feature unavailable from unsecured contexts'
) => {
  if (kIsUnsecured) await promise_rejects(testCase, code, promise, message);
  else await promise;
};

// These cases were initially based on the async cookies API interactive tests:
//
// https://raw.githubusercontent.com/WICG/async-cookies-api/gh-pages/cookies_test.js

function getOneSimpleOriginCookie() {
  return cookieStore.get('__Host-COOKIENAME').then(function(cookie) {
    if (!cookie) return undefined;
    return cookie.value;
  });
}

// Converts a list of cookie records {name, value} to [name=]value; ... as
// seen in Cookie: and document.cookie.
const cookieString = cookies => cookies.length ? cookies.map((
    {name, value}) => (name ? (name + '=') : '') + value).join('; ') :
      undefined;

// Approximate async equivalent to the document.cookie getter but with
// important differences: optional additional getAll arguments are
// forwarded, and an empty cookie jar returns undefined.
//
// This is intended primarily for verification against expected cookie
// jar contents. It should produce more readable messages using
// assert_equals in failing cases than assert_object_equals would
// using parsed cookie jar contents and also allows expectations to be
// written more compactly.
const getCookieString = async (...args) => {
  return cookieString(await cookieStore.getAll(...args));
}

// Approximate async equivalent to the document.cookie getter but from
// the server's point of view. Returns UTF-8 interpretation. Allows
// sub-path to be specified.
//
// Unlike document.cookie, this returns undefined when no cookies are
// present.
const getCookieStringHttp = async (extraPath = null) => {
  if (kIsStatic) throw 'CGI not available in static HTML test';
  const url =
        kCookieHelperCgi + ((extraPath == null) ? '' : ('/' + extraPath));
  const response = await fetch(url, { credentials: 'include' });
  const text = await response.text();
  assert_equals(
      response.ok,
      true,
      'CGI should have succeeded in getCookieStringHttp\n' + text);
  assert_equals(
      response.headers.get('content-type'),
      'text/plain; charset=utf-8',
      'CGI did not return UTF-8 text in getCookieStringHttp');
  if (text === '') return undefined;
  assert_equals(
      text.indexOf('cookie='),
      0,
      'CGI response did not begin with "cookie=" and was not empty: ' + text);
  return decodeURIComponent(text.replace(/^cookie=/, ''));
}

// Approximate async equivalent to the document.cookie getter but from
// the server's point of view. Returns binary string
// interpretation. Allows sub-path to be specified.
//
// Unlike document.cookie, this returns undefined when no cookies are
// present.
const getCookieBinaryHttp = async (extraPath = null) => {
  if (kIsStatic) throw 'CGI not available in static HTML test';
  const url =
        kCookieHelperCgi +
        ((extraPath == null) ?
         '' :
         ('/' + extraPath)) + '?charset=iso-8859-1';
  const response = await fetch(url, { credentials: 'include' });
  const text = await response.text();
  assert_equals(
      response.ok,
      true,
      'CGI should have succeeded in getCookieBinaryHttp\n' + text);
  assert_equals(
      response.headers.get('content-type'),
      'text/plain; charset=iso-8859-1',
      'CGI did not return ISO 8859-1 text in getCookieBinaryHttp');
  if (text === '') return undefined;
  assert_equals(
      text.indexOf('cookie='),
      0,
      'CGI response did not begin with "cookie=" and was not empty: ' + text);
  return unescape(text.replace(/^cookie=/, ''));
}

// Approximate async equivalent to the document.cookie setter but from
// the server's point of view.
const setCookieStringHttp = async setCookie => {
  if (kIsStatic) throw 'CGI not available in static HTML test';
  const encodedSetCookie = encodeURIComponent(setCookie);
  const url = kCookieHelperCgi;
  const headers = new Headers();
  headers.set(
      'content-type',
      'application/x-www-form-urlencoded; charset=utf-8');
  const response = await fetch(
      url,
      {
        credentials: 'include',
        method: 'POST',
        headers: headers,
        body: 'set-cookie=' + encodedSetCookie,
      });
  const text = await response.text();
  assert_equals(
      response.ok,
      true,
      'CGI should have succeeded in setCookieStringHttp set-cookie: ' +
        setCookie + '\n' + text);
  assert_equals(
      response.headers.get('content-type'),
      'text/plain; charset=utf-8',
      'CGI did not return UTF-8 text in setCookieStringHttp');
  assert_equals(
      text,
      'set-cookie=' + encodedSetCookie,
      'CGI did not faithfully echo the set-cookie value');
};

// Approximate async equivalent to the document.cookie setter but from
// the server's point of view. This version sets a binary cookie rather
// than a UTF-8 one.
const setCookieBinaryHttp = async setCookie => {
  if (kIsStatic) throw 'CGI not available in static HTML test';
  const encodedSetCookie = escape(setCookie).split('/').join('%2F');
  const url = kCookieHelperCgi + '?charset=iso-8859-1';
  const headers = new Headers();
  headers.set(
      'content-type',
      'application/x-www-form-urlencoded; charset=iso-8859-1');
  const response = await fetch(url, {
    credentials: 'include',
    method: 'POST',
    headers: headers,
    body: 'set-cookie=' + encodedSetCookie
  });
  const text = await response.text();
  assert_equals(
      response.ok,
      true,
      'CGI should have succeeded in setCookieBinaryHttp set-cookie: ' +
        setCookie + '\n' + text);
  assert_equals(
      response.headers.get('content-type'),
      'text/plain; charset=iso-8859-1',
      'CGI did not return Latin-1 text in setCookieBinaryHttp');
  assert_equals(
      text,
      'set-cookie=' + encodedSetCookie,
      'CGI did not faithfully echo the set-cookie value');
};

// Approximate async equivalent to the document.cookie setter but using
// <meta http-equiv="set-cookie" content="...">
const setCookieStringMeta = async setCookie => {
  const meta = Object.assign(document.createElement('meta'), {
    httpEquiv: 'set-cookie',
    content: setCookie
  });
  document.head.appendChild(meta);
  await new Promise(resolve => requestAnimationFrame(resolve));
  meta.parentNode.removeChild(meta);
};

// Async document.cookie getter; converts '' to undefined which loses
// information in the edge case where a single ''-valued anonymous
// cookie is visible.
const getCookieStringDocument = async () => {
  if (!kHasDocument) throw 'document.cookie not available in this context';
  return String(document.cookie || '') || undefined;
};

// Async document.cookie setter
const setCookieStringDocument = async setCookie => {
  if (!kHasDocument) throw 'document.cookie not available in this context';
  document.cookie = setCookie;
};
