#!/usr/bin/env python
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

def main(request, response):
  assert request.method in (
      'GET',
      'POST',
  ), 'request method was neither GET nor POST: %r' % request.method
  qd = (request.url.split('#')[0].split('?', 1) + [''])[1]
  if request.method == 'POST':
    qd += '&' + request.body
  args = cgi.parse_qs(qd, keep_blank_values = True)
  charset = encodings.codecs.lookup(args.get('charset', ['utf-8'])[-1]).name
  charset = CHARSET_OVERRIDES.get(charset, charset)
  headers = [('content-type', 'text/plain; charset=' + charset)]
  body = []
  if request.method == 'POST':
    for set_cookie in args.get('set-cookie', []):
      if '=' in set_cookie.split(';', 1)[0]:
        name, rest = set_cookie.split('=', 1)
        assert re.search(
            DISALLOWED_IN_COOKIE_NAME_RE,
            name
        ) is None, 'name had disallowed characters: %r' % name
      else:
        rest = set_cookie
      assert re.search(
          DISALLOWED_IN_HEADER_RE,
          rest
      ) is None, 'rest had disallowed characters: %r' % rest
      headers.append(('set-cookie', set_cookie))
      body.append('set-cookie=' + urllib.quote(set_cookie, ''))
  else:
    cookie = request.headers.get('cookie')
    if cookie is not None:
      body.append('cookie=' + urllib.quote(cookie, ''))
  body = '\r\n'.join(body)
  headers.append(('content-length', str(len(body))))
  return 200, headers, body

def cgi_main(env, fp):
  class CgiRequest(object):
    def __init__(self, env, fp):
      self.method = env['REQUEST_METHOD']
      self.url = env['SCRIPT_NAME'] + env.get('PATH_TRANSLATED', '')
      qs = env.get('QUERY_STRING', None)
      if qs is not None:
        self.url += '?' + qs
      self.headers = {
          k[len('HTTP_'):].lower(): v
          for k, v in env.items()
          if k.startswith('HTTP_')
      }
      self.body = None
      if self.method == 'POST':
        self.body = fp.read()
  status, headers, body = main(CgiRequest(env, fp), None)
  sys.stdout.write(''.join(line + '\r\n' for line in [
      'status: %s' % status
  ] + [
      ('' + header[0] + ': %s') % header[1]
      for header in headers
  ] + ['']) + body)

if __name__ == '__main__':
  cgi_main(os.environ, sys.stdin)
