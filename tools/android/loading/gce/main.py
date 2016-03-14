# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import re
import threading

from gcloud import storage
from gcloud.exceptions import NotFound
from oauth2client.client import GoogleCredentials

class ServerApp(object):
  """Simple web server application, collecting traces and writing them in
  Google Cloud Storage.
  """

  def __init__(self):
    self._tasks = []
    self._thread = None
    print 'Initializing credentials'
    self._credentials = GoogleCredentials.get_application_default()
    print 'Reading server configuration'
    with open('server_config.json') as configuration_file:
       self._config = json.load(configuration_file)

  def _GetStorageClient(self):
    return storage.Client(project = self._config['project_name'],
                          credentials = self._credentials)

  def _GetStorageBucket(self, storage_client):
    return storage_client.get_bucket(self._config['bucket_name'])

  def _UploadFile(self, file_stream, filename):
    client = self._GetStorageClient()
    bucket = self._GetStorageBucket(client)
    blob = bucket.blob(filename)
    blob.upload_from_string(file_stream)
    url = blob.public_url
    return url

  def _DeleteFile(self, filename):
    client = self._GetStorageClient()
    bucket = self._GetStorageBucket(client)
    try:
      bucket.delete_blob(filename)
      return True
    except NotFound:
      return False

  def _ReadFile(self, filename):
    client = self._GetStorageClient()
    bucket = self._GetStorageBucket(client)
    blob = bucket.get_blob(filename)
    if not blob:
      return None
    return blob.download_as_string()

  def _SetTasks(self, task_list):
    if len(self._tasks) > 0:
      return False  # There are tasks already.
    self._tasks = json.loads(task_list)
    return len(self._tasks) != 0

  def _ProcessTasks(self):
    # Avoid special characters in storage object names
    pattern = re.compile(r"[#\?\[\]\*/]")
    while len(self._tasks) > 0:
      url = self._tasks.pop()
      filename = pattern.sub('_', url)
      # TODO: compute the actual trace for url.
      trace = '{}'
      self._UploadFile(trace, filename)

  def __call__(self, environ, start_response):
    path = environ['PATH_INFO']
    if path == '/favicon.ico':
        start_response('404 NOT FOUND', [('Content-Length', '0')])
        return iter([''])

    status = '200 OK'

    if path == '/set_tasks':
      # Get the tasks from the HTTP body.
      try:
        body_size = int(environ.get('CONTENT_LENGTH', 0))
      except (ValueError):
        body_size = 0
      body = environ['wsgi.input'].read(body_size)
      if self._SetTasks(body):
        data = 'Set tasks: ' + str(len(self._tasks))
      else:
        data = 'Something went wrong'
    elif path == '/start':
      if len(self._tasks) > 0:
        data = 'Starting...'
        self._thread = threading.Thread(target = self._ProcessTasks)
        self._thread.start()
      else:
        data = 'Nothing to do!'
    else:
      data = environ['PATH_INFO'] + '\n'

    response_headers = [
        ('Content-type','text/plain'),
        ('Content-Length', str(len(data)))
    ]
    start_response(status, response_headers)
    return iter([data])


app = ServerApp()
