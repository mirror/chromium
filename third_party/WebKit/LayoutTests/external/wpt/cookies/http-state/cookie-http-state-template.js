function strip_prefix_and_whitespace(cookie_text) {
  return cookie_text.replace(/^Cookie: /, '').replace(/^\s+|\s+$/g, '');
}

function CookieDiffTool() {
  this.initial_cookies = [];
}

CookieDiffTool.prototype.parse = (document_cookies) => {
  this.initial_cookies = document_cookies
      .replace(/^Cookie: /, '')
      .split(/\s*;\s*/);
}

CookieDiffTool.prototype.diff_with = (document_cookies) => {
  let actual_cookies = document_cookies;
  for (let i in initial_cookies) {
    let no_spaces_cookie_regex =
        new RegExp(/\s*[\;]*\s/.source + initial_cookies[i]);
    actual_cookies = actual_cookies.replace(no_spaces_cookie_regex, '');
  }
  return actual_cookies;
}

function create_cookie_test(file) {
  return t => {
    const iframe = document.createElement('iframe');
    document.body.appendChild(iframe);
    let diff_tool = new CookieDiffTool();
    return new Promise((resolve, reject) => {
      iframe.onload = () => {
        resolve(iframe.contentDocument.body.innerText);
      };
      diff_tool.parse(iframe.contentDocument.cookie);
      iframe.src = "./resources/cookie-setter.py?file=0002";
    }).then((expectation_text) => {
      let actual_cookies = diff_tool.diff_with(iframe.contentDocument.cookie);
      let expected_cookies = strip_prefix_and_whitespace(expectation_text);
      assert_equals(actual_cookies, expected_cookies);
      t.done();
    });
  }
};
