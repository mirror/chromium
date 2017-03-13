# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import urllib
import urllib2
import requests
import base64

from webkitpy.common.net.web import Web
from webkitpy.w3c.chromium_commit import ChromiumCommit
from webkitpy.w3c.chromium_finder import absolute_chromium_dir
from webkitpy.w3c.test_exporter import TestExporter

_log = logging.getLogger(__name__)
URL_BASE = 'https://chromium-review.googlesource.com'

def api_get(url):
    web = Web()
    raw_data = web.get_binary(url)
    return json.loads(raw_data[5:])  # strip JSONP preamble

def cl_url(number):
    return 'https://chromium-review.googlesource.com/c/{}/'.format(number)


def exportable_filename(filename):
    filename = os.path.basename(filename.lower())
    return (
        not filename.endswith('-expected.txt')
        and not filename.startswith('.')
        and not filename.endswith('.json')
    )

def get_diff(cl):
    revision = cl['revisions'].items()[0][1]
    revision_id = cl['revisions'].items()[0][0]
    files = revision['files'].keys()

    diff = ''

    print 'Getting the diff'

    for file in files:
        url = '{url_base}/changes/{change_id}/revisions/{revision_id}/files/{file_id}/diff'.format(
            url_base=URL_BASE,
            change_id=cl['id'],
            revision_id=revision_id,
            file_id=urllib.quote_plus(file),
        )
        data = api_get(url)
        print 'DIFF DATA'
        print data

    return

def post_comment(cl_id, revision_id, message):
    # CL_ID = 448684
    # revision_id = '8b2ae5c5be88db86c1a86ebc017d6054a3f2c992'
    # TODO(jeffcarp): need to get the latest revision id

    url = '{url_base}/a/changes/{change_id}/revisions/{revision_id}/review'.format(
        url_base=URL_BASE,
        change_id=cl_id,
        revision_id=revision_id,
    )
    data = { 'message': message }

    # domain = '.googlesource.com'
    domain = 'TODO: pass this in via command line or read .gitcookies'
    token = 'TODO: pass this in via command line or read .gitcookies'

    b64auth = base64.b64encode('{}:{}'.format(domain, token))
    headers = {
        'Authorization': 'Basic {}'.format(b64auth)
    }
    print headers

    requests.post(url, data=data, auth=(domain, token))

def main(host):
    exportable_cls = query_open_cls()
    test_exporter = TestExporter(
        host=host,
        gh_user='TODO: pass this in via args',
        gh_token='TODO: pass this in via args',
    )
    print exportable_cls

    for cl in exportable_cls:

        # TODO(jeffcarp): Here check if there's an in-flight CL on GitHub

        # Apply patch
        branch_name = 'chromium-export-cl-{}'.format(cl['_number'])
        host.executive.run_command(['git', 'cl', 'patch', cl['_number'], '-b', branch_name, '--gerrit'])

        # TODO(jeffcarp): Clean up the tree after this completes
        #   Maybe wrap in a try catch to make sure things are cleaned up?

        sha = host.executive.run_command([
            'git', 'show', '--format=%H', '--no-patch'
        ], cwd=absolute_chromium_dir(host)).strip()

        print sha
        exportable_commit = ChromiumCommit(host=host, sha=sha)
        print exportable_commit

        # TODO(jeffcarp): create a test patch to make sure it diffs anything

        pull_request = test_exporter.export_commit(exportable_commit)

        # Clean up
        host.executive.run_command([
            'git', 'checkout', '-'
        ], cwd=absolute_chromium_dir(host))
        host.executive.run_command([
            'git', 'branch', '-D', branch_name
        ], cwd=absolute_chromium_dir(host))

        # Post a comment on the CL with the link to the GitHub PR
        pr_url = 'https://github.com/w3c/web-platform-tests/pull/{}'.format(pull_request['number'])
        message = 'Upstreamable changes to web-platform-tests were detected in this CL and a PR has been made! {} New patches to this CL will automatically update the PR. We will notify you here if Travis CI fails. If this CL lands and Travis CI is green, the WPT PR will be merged.'.format(pr_url)

        # TODO(jeffcarp): handle multiple revisions e.g. if a newer one is uploaded
        revision_id = cl['revisions'].items()[0][0]

        post_comment(cl['_number'], revision_id, message)

        break

        '''
        - Create ChromiumCommit for HEAD
            - Possible issue: what if change is >1 commit?
            - I don't think that would happen using git cl patch

        - Create a WPT PR off of that branch, use existing export infra
        '''

def query_open_cls(limit=200):
    url = "{}/changes/?q=project:\"chromium/src\"+status:open&o=CURRENT_FILES&o=CURRENT_REVISION&o=COMMIT_FOOTERS&n={}".format(URL_BASE, limit)
    WPT_DIRECTORY = 'third_party/WebKit/LayoutTests/external/wpt'
    web = Web()
    raw_data = web.get_binary(url)

    # TODO(jeffcarp): If this fails, we should fail the entire script and fire an alert

    data = json.loads(raw_data[5:])  # strip JSONP preamble
    print 'Received {} records.'.format(len(data))

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

        files_in_wpt = [f for f in files if f.startswith(WPT_DIRECTORY)]
        if not files_in_wpt:
            continue

        exportable_files = [f for f in files_in_wpt if exportable_filename(f)]

        if not exportable_files:
            continue

        exportable_cls.append(cl)

    return exportable_cls
