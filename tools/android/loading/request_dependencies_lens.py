# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Gathers and infers dependencies between requests.

When executed as a script, loads a trace and outputs the dependencies.
"""

import collections
import logging
import operator

import loading_trace
import request_track


class RequestDependencyLens(object):
  """Analyses and infers request dependencies."""
  DEPENDENCIES = ('redirect', 'parser', 'script', 'inferred', 'other')
  def __init__(self, trace):
    """Initializes an instance of RequestDependencyLens.

    Args:
      trace: (LoadingTrace) Loading trace.
    """
    self.loading_trace = trace
    self._requests = self.loading_trace.request_track.GetEvents()
    self._requests_by_id = {r.request_id: r for r in self._requests}
    self._requests_by_url = collections.defaultdict(list)
    for request in self._requests:
      self._requests_by_url[request.url].append(request)
    self._frame_to_parent = {}
    for event in self.loading_trace.page_track.GetEvents():
      if event['method'] == 'Page.frameAttached':
        self._frame_to_parent[event['frame_id']] = event['parent_frame_id']

  def GetRequestDependencies(self):
    """Returns a list of request dependencies.

    Returns:
      [(first, second, reason), ...] where first and second are instances of
      request_track.Request, and reason is in DEPENDENCIES. The second request
      depends on the first one, with the listed reason.
    """
    deps = []
    for request in self._requests:
      dependency = self._GetDependency(request)
      if dependency:
        deps.append(dependency)
    return deps

  def _GetDependency(self, request):
    """Returns (first, second, reason), or None.

    |second| depends on |first|.

    Args:
      request: (Request) the request we wish to get the initiator of.

    Returns:
      None if no dependency is found from this request, or
      (initiator (Request), blocked_request (Request), reason (str)).
    """
    reason = request.initiator['type']
    assert reason in request_track.Request.INITIATORS
    # Redirect suffixes are added in RequestTrack.
    if request.request_id.endswith(request_track.RequestTrack.REDIRECT_SUFFIX):
      return self._GetInitiatingRequestRedirect(request)
    elif reason == 'parser':
      return self._GetInitiatingRequestParser(request)
    elif reason == 'script':
      return self._GetInitiatingRequestScript(request)
    else:
      assert reason == 'other'
      return self._GetInitiatingRequestOther(request)

  def _GetInitiatingRequestRedirect(self, request):
    request_id = request.request_id[:request.request_id.index(
        request_track.RequestTrack.REDIRECT_SUFFIX)]
    assert request_id in self._requests_by_id
    dependent_request = self._requests_by_id[request_id]
    assert request.timing.request_time < \
        dependent_request.timing.request_time, '.\n'.join(
            [str(request), str(dependent_request)])
    return (request, dependent_request, 'redirect')

  def _GetInitiatingRequestParser(self, request):
    url = request.initiator['url']
    candidates = self._FindMatchingRequests(url, request.timing.request_time)
    if not candidates:
      return None
    initiating_request = self._FindBestMatchingInitiator(request, candidates)
    return (initiating_request, request, 'parser')

  def _GetInitiatingRequestScript(self, request):
    if not 'stackTrace' in request.initiator:
      logging.warning('Script initiator but no stack trace.')
      return None
    initiating_request = None
    timestamp = request.timing.request_time
    for frame in request.initiator['stackTrace']:
      url = frame['url']
      candidates = self._FindMatchingRequests(url, timestamp)
      if candidates:
        initiating_request = self._FindBestMatchingInitiator(
            request, candidates)
        if initiating_request:
          break
    else:
      for frame in request.initiator['stackTrace']:
        if not frame.get('url', None) and frame.get(
            'functionName', None) == 'window.onload':
          logging.warning('Unmatched request for onload handler.')
          break
      else:
        logging.warning('Unmatched request.')
      return None
    return (initiating_request, request, 'script')

  def _GetInitiatingRequestOther(self, _):
    # TODO(lizeb): Infer "other" initiator types.
    return None

  def _FindMatchingRequests(self, url, before_timestamp):
    """Returns a list of requests matching a URL, before a timestamp.

    Args:
      url: (str) URL to match in requests.
      before_timestamp: (int) Only keep requests submitted before a given
                        timestamp.

    Returns:
      A list of candidates, ordered by timestamp.
    """
    candidates = self._requests_by_url.get(url, [])
    candidates = [r for r in candidates if (
        r.timing.request_time + max(
            0, r.timing.receive_headers_end / 1000) <= before_timestamp)]
    candidates.sort(key=lambda r: r.timing.request_time)
    return candidates

  def _FindBestMatchingInitiator(self, request, matches):
    """Returns the best matching request within a list of matches.

    Iteratively removes candidates until one is left:
    - With the same parent frame.
    - From the same frame.

    If this is not successful, takes the most recent request.

    Args:
      request: (Request) Request.
      matches: [Request] As returned by _FindMatchingRequests(), that is
               sorted by timestamp.

    Returns:
      The best matching initiating request, or None.
    """
    if not matches:
      return None
    if len(matches) == 1:
      return matches[0]
    # Several matches, try to reduce this number to 1. Otherwise, return the
    # most recent one.
    if request.frame_id in self._frame_to_parent: # Main frame has no parent.
      parent_frame_id = self._frame_to_parent[request.frame_id]
      same_parent_matches = [
          r for r in matches
          if r.frame_id in self._frame_to_parent and
          self._frame_to_parent[r.frame_id] == parent_frame_id]
      if not same_parent_matches:
        logging.warning('All matches are from non-sibling frames.')
        return matches[-1]
      if len(same_parent_matches) == 1:
        return same_parent_matches[0]
    same_frame_matches = [r for r in matches if r.frame_id == request.frame_id]
    if not same_frame_matches:
      logging.warning('All matches are from non-sibling frames.')
      return matches[-1]
    if len(same_frame_matches) == 1:
      return same_frame_matches[0]
    else:
      logging.warning('Several matches')
      return same_frame_matches[-1]


if __name__ == '__main__':
  import json
  import sys
  trace_filename = sys.argv[1]
  json_dict = json.load(open(trace_filename, 'r'))
  lens = RequestDependencyLens(
      loading_trace.LoadingTrace.FromJsonDict(json_dict))
  depedencies = lens.GetRequestDependencies()
  for (first, second, dep_reason) in depedencies:
    print '%s -> %s\t(%s)' % (first.request_id, second.request_id, dep_reason)
