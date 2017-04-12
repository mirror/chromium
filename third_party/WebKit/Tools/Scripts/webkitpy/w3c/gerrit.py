# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import urllib
import base64

_log = logging.getLogger(__name__)
URL_BASE = 'https://chromium-review.googlesource.com'


class Gerrit(object):

    def __init__(self, host, user, token):
        self.host = host
        self.user = user
        self.token = token

    def api_get(self, url):
        raw_data = self.host.web.get_binary(url)
        return json.loads(raw_data[5:])  # strip JSONP preamble

    def api_post(self, url, data):
        if not (self.user and self.token):
            raise 'Gerrit user and token requried for authenticated routes.'

        b64auth = base64.b64encode('{}:{}'.format(self.user, self.token))
        headers = {
            'Authorization': 'Basic {}'.format(b64auth),
            'Content-Type': 'application/json',
        }
        return self.host.web.request('POST', url, data=json.dumps(data), headers=headers)

    def cl_url(self, number):
        return 'https://chromium-review.googlesource.com/c/{}/'.format(number)

    def exportable_filename(self, filename):
        filename = os.path.basename(filename.lower())
        return (
            not filename.endswith('-expected.txt')
            and not filename.startswith('.')
            and not filename.endswith('.json')
        )

    def get_diff(self, cl):
        """Get full diff for latest revision of CL."""
        revision = cl['revisions'].items()[0][1]
        revision_id = cl['revisions'].items()[0][0]
        files = revision['files'].keys()
        diff = ''

        for filename in files:
            url = '{url_base}/changes/{change_id}/revisions/{revision_id}/files/{file_id}/diff'.format(
                url_base=URL_BASE,
                change_id=cl['id'],
                revision_id=revision_id,
                file_id=urllib.quote_plus(filename),
            )
            data = self.api_get(url)
            diff += data

        return diff

    def post_comment(self, cl_id, revision_id, message):
        url = '{url_base}/a/changes/{change_id}/revisions/{revision_id}/review'.format(
            url_base=URL_BASE,
            change_id=cl_id,
            revision_id=revision_id,
        )
        return self.api_post(url, {'message': message})

    def query_open_cls(self, limit=200):
        url = ("{}/changes/?q=project:\"chromium/src\"+status:open"
               "&o=CURRENT_FILES&o=CURRENT_REVISION&o=COMMIT_FOOTERS&n={}").format(URL_BASE, limit)
        data = self.api_get(url)
        exportable_cls = []

        for cl in data:
            revision = cl['revisions'].items()[0][1]
            files = revision['files'].keys()

            if cl['subject'].startswith('Import wpt@'):
                continue

            if 'Import' in cl['subject']:
                continue

            if 'NOEXPORT=true' in revision['commit_with_footers']:
                continue

            files_in_wpt = [f for f in files if f.startswith('third_party/WebKit/LayoutTests/external/wpt')]
            if not files_in_wpt:
                continue

            exportable_files = [f for f in files_in_wpt if self.exportable_filename(f)]

            if not exportable_files:
                continue

            exportable_cls.append(cl)

        return exportable_cls
