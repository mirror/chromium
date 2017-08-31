#!/usr/bin/python
# -*- coding: utf-8 -*-

import cgi, encodings, os, re, sys, urllib

# NOTE: These are intentionally very lax to permit testing
DISALLOWED_IN_COOKIE_NAME_RE = re.compile(r'[;\0-\x1f\x7f]');
DISALLOWED_IN_HEADER_RE = re.compile(r'[\0-\x1f\x7f]');

# Ensure common charset names do not end up with different
# capitalization or punctuation
CHARSET_OVERRIDES = {
    encodings.codecs.lookup(charset).name: charset
    for charset in ('utf-8', 'iso-8859-1',)
}

def main(env):
  args = cgi.parse(fp = sys.stdin, environ = env, keep_blank_values = True)
  request_method = env.get('REQUEST_METHOD', 'GET')
  assert request_method in ('GET', 'POST',), 'request method was neither GET nor POST: %r' % request_method
  charset = encodings.codecs.lookup(args.get('charset', ['utf-8'])[-1]).name
  charset = CHARSET_OVERRIDES.get(charset, charset)
  headers = [
      'status: 200',
      'content-type: text/plain; charset=' + charset,
  ]
  body = []
  if request_method == 'POST':
    for set_cookie in args.get('set-cookie', []):
      if '=' in set_cookie.split(';', 1)[0]:
        name, rest = set_cookie.split('=', 1)
        assert re.search(DISALLOWED_IN_COOKIE_NAME_RE, name) is None, 'name had disallowed characters: %r' % name
      else:
        rest = set_cookie
      assert re.search(DISALLOWED_IN_HEADER_RE, rest) is None, 'rest had disallowed characters: %r' % rest
      headers.append('set-cookie: ' + set_cookie)
      body.append('set-cookie=' + urllib.quote(set_cookie, ''))
  elif 'HTTP_COOKIE' in env:
    body.append('cookie=' + urllib.quote(env['HTTP_COOKIE'], ''))
  body = '\r\n'.join(body)
  headers.append('content-length: %d' % len(body))
  sys.stdout.write(''.join(line + '\r\n' for line in headers + ['', body]))

if __name__ == '__main__':
  main(os.environ)
