# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The request data track.

When executed, parses a JSON dump of DevTools messages.
"""

import collections
import copy
import json
import logging
import re

import devtools_monitor


_TIMING_NAMES_MAPPING = {
    'connectEnd': 'connect_end', 'connectStart': 'connect_start',
    'dnsEnd': 'dns_end', 'dnsStart': 'dns_start', 'proxyEnd': 'proxy_end',
    'proxyStart': 'proxy_start', 'receiveHeadersEnd': 'receive_headers_end',
    'requestTime': 'request_time', 'sendEnd': 'send_end',
    'sendStart': 'send_start', 'sslEnd': 'ssl_end', 'sslStart': 'ssl_start',
    'workerReady': 'worker_ready', 'workerStart': 'worker_start',
    'loadingFinished': 'loading_finished'}

Timing = collections.namedtuple('Timing', _TIMING_NAMES_MAPPING.values())

def TimingAsList(timing):
  """Transform Timing to a list, eg as is used in JSON output.

  Args:
    timing: a Timing.

  Returns:
    A list identical to what the eventual JSON output will be (eg,
    Request.ToJsonDict).
  """
  return json.loads(json.dumps(timing))

class Request(object):
  """Represents a single request.

  Generally speaking, fields here closely mirror those documented in
  third_party/WebKit/Source/devtools/protocol.json.

  Fields:
    request_id: (str) unique request ID. Postfixed with _REDIRECT_SUFFIX for
                redirects.
    frame_id: (str) unique frame identifier.
    loader_id: (str) unique frame identifier.
    document_url: (str) URL of the document this request is loaded for.
    url: (str) Request URL.
    protocol: (str) protocol used for the request.
    method: (str) HTTP method, such as POST or GET.
    request_headers: (dict) {'header': 'value'} Request headers.
    response_headers: (dict) {'header': 'value'} Response headers.
    initial_priority: (str) Initial request priority, in REQUEST_PRIORITIES.
    timestamp: (float) Request timestamp, in s.
    wall_time: (float) Request timestamp, UTC timestamp in s.
    initiator: (dict) Request initiator, in INITIATORS.
    resource_type: (str) Resource type, in RESOURCE_TYPES
    served_from_cache: (bool) Whether the request was served from cache.
    from_disk_cache: (bool) Whether the request was served from the disk cache.
    from_service_worker: (bool) Whether the request was served by a Service
                         Worker.
    timing: (Timing) Request timing, extended with loading_finished.
    status: (int) Response status code.
    encoded_data_length: (int) Total encoded data length.
    data_chunks: (list) [(offset, encoded_data_length), ...] List of data
                 chunks received, with their offset in ms relative to
                 Timing.requestTime.
    failed: (bool) Whether the request failed.
  """
  REQUEST_PRIORITIES = ('VeryLow', 'Low', 'Medium', 'High', 'VeryHigh')
  RESOURCE_TYPES = ('Document', 'Stylesheet', 'Image', 'Media', 'Font',
                    'Script', 'TextTrack', 'XHR', 'Fetch', 'EventSource',
                    'WebSocket', 'Manifest', 'Other')
  INITIATORS = ('parser', 'script', 'other', 'redirect')
  INITIATING_REQUEST = 'initiating_request'
  ORIGINAL_INITIATOR = 'original_initiator'
  def __init__(self):
    self.request_id = None
    self.frame_id = None
    self.loader_id = None
    self.document_url = None
    self.url = None
    self.protocol = None
    self.method = None
    self.request_headers = None
    self.response_headers = None
    self.initial_priority = None
    self.timestamp = -1
    self.wall_time = -1
    self.initiator = None
    self.resource_type = None
    self.served_from_cache = False
    self.from_disk_cache = False
    self.from_service_worker = False
    self.timing = None
    self.status = None
    self.encoded_data_length = 0
    self.data_chunks = []
    self.failed = False

  def _TimestampOffsetFromStartMs(self, timestamp):
    assert self.timing.request_time != -1
    request_time = self.timing.request_time
    return (timestamp - request_time) * 1000

  def ToJsonDict(self):
    return copy.deepcopy(self.__dict__)

  @classmethod
  def FromJsonDict(cls, data_dict):
    result = Request()
    for (k, v) in data_dict.items():
      setattr(result, k, v)
    if not result.response_headers:
      result.response_headers = {}
    if result.timing:
      result.timing = Timing(*result.timing)
    else:
      result.timing = TimingFromDict({'requestTime': result.timestamp})
    return result

  def GetContentType(self):
    """Returns the content type, or None."""
    content_type = self.response_headers.get('Content-Type', None)
    if not content_type or ';' not in content_type:
      return content_type
    else:
      return content_type[:content_type.index(';')]

  def IsDataRequest(self):
    return self.protocol == 'data'

  def MaxAge(self):
    """Returns the max-age of a resource, or -1."""
    # TODO(lizeb): Handle the "Expires" header as well.
    cache_control = {}
    if not self.response_headers:
      return -1
    cache_control_str = self.response_headers.get('Cache-Control', None)
    if cache_control_str is not None:
      directives = [s.strip() for s in cache_control_str.split(',')]
      for directive in directives:
        parts = [s.strip() for s in directive.split('=')]
        if len(parts) == 1:
          cache_control[parts[0]] = True
        else:
          cache_control[parts[0]] = parts[1]
    if (u'no-store' in cache_control
        or u'no-cache' in cache_control
        or len(cache_control) == 0):
      return -1
    if 'max-age' in cache_control:
      age_match = re.match(r'\s*(\d+)+', cache_control['max-age'])
      if not age_match:
        return -1
      return int(age_match.group(1))
    return -1

  def __eq__(self, o):
    return self.__dict__ == o.__dict__

  def __hash__(self):
    return hash(self.request_id)

  def __str__(self):
    return json.dumps(self.ToJsonDict(), sort_keys=True, indent=2)


class RequestTrack(devtools_monitor.Track):
  """Aggregates request data."""
  _REDIRECT_SUFFIX = '.redirect'
  # Request status
  _STATUS_SENT = 0
  _STATUS_RESPONSE = 1
  _STATUS_DATA = 2
  _STATUS_FINISHED = 3
  _STATUS_FAILED = 4
  # Serialization KEYS
  _EVENTS_KEY = 'events'
  _METADATA_KEY = 'metadata'
  _DUPLICATES_KEY = 'duplicates_count'
  _INCONSISTENT_INITIATORS_KEY = 'inconsistent_initiators'
  def __init__(self, connection):
    super(RequestTrack, self).__init__(connection)
    self._connection = connection
    self._requests = []
    self._requests_in_flight = {}  # requestId -> (request, status)
    self._completed_requests_by_id = {}
    self._redirects_count_by_id = collections.defaultdict(int)
    if connection:  # Optional for testing.
      for method in RequestTrack._METHOD_TO_HANDLER:
        self._connection.RegisterListener(method, self)
    # responseReceived message are sometimes duplicated. Records the message to
    # detect this.
    self._request_id_to_response_received = {}
    self.duplicates_count = 0
    self.inconsistent_initiators_count = 0

  def Handle(self, method, msg):
    assert method in RequestTrack._METHOD_TO_HANDLER
    params = msg['params']
    request_id = params['requestId']
    RequestTrack._METHOD_TO_HANDLER[method](self, request_id, params)

  def GetEvents(self):
    if self._requests_in_flight:
      logging.warning('Number of requests still in flight: %d.'
                      % len(self._requests_in_flight))
    return self._requests

  def ToJsonDict(self):
    if self._requests_in_flight:
      logging.warning('Requests in flight, will be ignored in the dump')
    return {self._EVENTS_KEY: [
        request.ToJsonDict() for request in self._requests],
            self._METADATA_KEY: {
                self._DUPLICATES_KEY: self.duplicates_count,
                self._INCONSISTENT_INITIATORS_KEY:
                self.inconsistent_initiators_count}}

  @classmethod
  def FromJsonDict(cls, json_dict):
    assert cls._EVENTS_KEY in json_dict
    assert cls._METADATA_KEY in json_dict
    result = RequestTrack(None)
    requests = [Request.FromJsonDict(request)
                for request in json_dict[cls._EVENTS_KEY]]
    result._requests = requests
    metadata = json_dict[cls._METADATA_KEY]
    result.duplicates_count = metadata.get(cls._DUPLICATES_KEY, 0)
    result.inconsistent_initiators_count = metadata.get(
        cls._INCONSISTENT_INITIATORS_KEY, 0)
    return result

  def _RequestWillBeSent(self, request_id, params):
    # Several "requestWillBeSent" events can be dispatched in a row in the case
    # of redirects.
    redirect_initiator = None
    if request_id in self._requests_in_flight:
      redirect_initiator = self._HandleRedirect(request_id, params)
    assert (request_id not in self._requests_in_flight
            and request_id not in self._completed_requests_by_id)
    r = Request()
    r.request_id = request_id
    _CopyFromDictToObject(
        params, r, (('frameId', 'frame_id'), ('loaderId', 'loader_id'),
                    ('documentURL', 'document_url'),
                    ('timestamp', 'timestamp'), ('wallTime', 'wall_time'),
                    ('initiator', 'initiator')))
    request = params['request']
    _CopyFromDictToObject(
        request, r, (('url', 'url'), ('method', 'method'),
                     ('headers', 'headers'),
                     ('initialPriority', 'initial_priority')))
    r.resource_type = params.get('type', 'Other')
    if redirect_initiator:
      original_initiator = r.initiator
      r.initiator = redirect_initiator
      r.initiator[Request.ORIGINAL_INITIATOR] = original_initiator
      initiating_request = self._completed_requests_by_id[
          redirect_initiator[Request.INITIATING_REQUEST]]
      initiating_initiator = initiating_request.initiator.get(
          Request.ORIGINAL_INITIATOR, initiating_request.initiator)
      if initiating_initiator != original_initiator:
        self.inconsistent_initiators_count += 1
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_SENT)

  def _HandleRedirect(self, request_id, params):
    (r, status) = self._requests_in_flight[request_id]
    assert status == RequestTrack._STATUS_SENT
    # The second request contains timing information pertaining to the first
    # one. Finalize the first request.
    assert 'redirectResponse' in params
    redirect_response = params['redirectResponse']

    _CopyFromDictToObject(redirect_response, r,
                          (('headers', 'response_headers'),
                           ('encodedDataLength', 'encoded_data_length'),
                           ('fromDiskCache', 'from_disk_cache')))
    r.timing = TimingFromDict(redirect_response['timing'])

    redirect_index = self._redirects_count_by_id[request_id]
    self._redirects_count_by_id[request_id] += 1
    r.request_id = '%s%s.%d' % (request_id, self._REDIRECT_SUFFIX,
                                 redirect_index + 1)
    initiator = {
        'type': 'redirect', Request.INITIATING_REQUEST: r.request_id}
    self._requests_in_flight[r.request_id] = (r, RequestTrack._STATUS_FINISHED)
    del self._requests_in_flight[request_id]
    self._FinalizeRequest(r.request_id)
    return initiator

  def _RequestServedFromCache(self, request_id, _):
    assert request_id in self._requests_in_flight
    (request, status) = self._requests_in_flight[request_id]
    assert status == RequestTrack._STATUS_SENT
    request.served_from_cache = True

  def _ResponseReceived(self, request_id, params):
    assert request_id in self._requests_in_flight
    (r, status) = self._requests_in_flight[request_id]
    if status == RequestTrack._STATUS_RESPONSE:
      # Duplicated messages (apart from the timestamp) are OK.
      old_params = self._request_id_to_response_received[request_id]
      params_copy = copy.deepcopy(params)
      params_copy['timestamp'] = None
      old_params['timestamp'] = None
      assert params_copy == old_params
      self.duplicates_count += 1
      return
    assert status == RequestTrack._STATUS_SENT
    assert r.frame_id == params['frameId']
    assert r.timestamp <= params['timestamp']
    if r.resource_type == 'Other':
      r.resource_type = params.get('type', 'Other')
    else:
      assert r.resource_type == params.get('type', 'Other')
    response = params['response']
    _CopyFromDictToObject(
        response, r, (('status', 'status'), ('mimeType', 'mime_type'),
                      ('fromDiskCache', 'from_disk_cache'),
                      ('fromServiceWorker', 'from_service_worker'),
                      ('protocol', 'protocol'),
                      # Actual request headers are not known before reaching the
                      # network stack.
                      ('requestHeaders', 'request_headers'),
                      ('headers', 'response_headers')))
    # data URLs don't have a timing dict.
    timing_dict = {}
    if r.protocol != 'data':
      timing_dict = response['timing']
    else:
      timing_dict = {'requestTime': r.timestamp}
    r.timing = TimingFromDict(timing_dict)
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_RESPONSE)
    self._request_id_to_response_received[request_id] = params

  def _DataReceived(self, request_id, params):
    (r, status) = self._requests_in_flight[request_id]
    assert (status == RequestTrack._STATUS_RESPONSE
            or status == RequestTrack._STATUS_DATA)
    offset = r._TimestampOffsetFromStartMs(params['timestamp'])
    r.data_chunks.append((offset, params['encodedDataLength']))
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_DATA)

  def _LoadingFinished(self, request_id, params):
    assert request_id in self._requests_in_flight
    (r, status) = self._requests_in_flight[request_id]
    assert (status == RequestTrack._STATUS_RESPONSE
            or status == RequestTrack._STATUS_DATA)
    r.encoded_data_length = params['encodedDataLength']
    r.timing = r.timing._replace(
        loading_finished=r._TimestampOffsetFromStartMs(params['timestamp']))
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_FINISHED)
    self._FinalizeRequest(request_id)

  def _LoadingFailed(self, request_id, _):
    assert request_id in self._requests_in_flight
    (r, _) = self._requests_in_flight[request_id]
    r.failed = True
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_FINISHED)
    self._FinalizeRequest(request_id)

  def _FinalizeRequest(self, request_id):
    assert request_id in self._requests_in_flight
    (request, status) = self._requests_in_flight[request_id]
    assert status == RequestTrack._STATUS_FINISHED
    del self._requests_in_flight[request_id]
    self._completed_requests_by_id[request_id] = request
    self._requests.append(request)

  def __eq__(self, o):
    return self._requests == o._requests


RequestTrack._METHOD_TO_HANDLER = {
    'Network.requestWillBeSent': RequestTrack._RequestWillBeSent,
    'Network.requestServedFromCache': RequestTrack._RequestServedFromCache,
    'Network.responseReceived': RequestTrack._ResponseReceived,
    'Network.dataReceived': RequestTrack._DataReceived,
    'Network.loadingFinished': RequestTrack._LoadingFinished,
    'Network.loadingFailed': RequestTrack._LoadingFailed}


def TimingFromDict(timing_dict):
  """Returns an instance of Timing from an () dict."""
  complete_timing_dict = {field: -1 for field in Timing._fields}
  timing_dict_mapped = {
      _TIMING_NAMES_MAPPING[k]: v for (k, v) in timing_dict.items()}
  complete_timing_dict.update(timing_dict_mapped)
  return Timing(**complete_timing_dict)


def _CopyFromDictToObject(d, o, key_attrs):
  for (key, attr) in key_attrs:
    if key in d:
      setattr(o, attr, d[key])


if __name__ == '__main__':
  import json
  import sys
  events = json.load(open(sys.argv[1], 'r'))
  request_track = RequestTrack(None)
  for event in events:
    event_method = event['method']
    request_track.Handle(event_method, event)
