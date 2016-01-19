# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import unittest

import devtools_monitor

from tracing import TracingTrack


class StubConnection(object):
  def RegisterListener(self, name, obj):
    pass

  def SyncRequestNoResponse(self, method, params):
    pass


class TracingTrackTestCase(unittest.TestCase):
  def setUp(self):
    self.track = TracingTrack(StubConnection())

  def EventToMicroseconds(self, event):
    if 'ts' in event:
      event['ts'] *= 1000
    if 'dur' in event:
      event['dur'] *= 1000
    return event

  def CheckTrack(self, timestamp, names):
    self.assertEqual(
        set((e.args['name'] for e in self.track.EventsAt(timestamp))),
        set(names))

  def CheckIntervals(self, events):
    """All tests should produce the following sequence of intervals, each
    identified by a 'name' in the event args.

    Timestamp
    3    |      A
    4    |
    5    | |    B
    6    |
    7
    ..
    10   | |    C, D
    11     |
    12   |      E
    13   | |    F
    14   |
    """
    self.track.Handle('Tracing.dataCollected',
                      {'params': {'value': [self.EventToMicroseconds(e)
                                            for e in events]}})
    self.CheckTrack(0, '')
    self.CheckTrack(2, '')
    self.CheckTrack(3, 'A')
    self.CheckTrack(4, 'A')
    self.CheckTrack(5, 'AB')
    self.CheckTrack(6, 'A')
    self.CheckTrack(7, '')
    self.CheckTrack(9, '')
    self.CheckTrack(10, 'CD')
    self.CheckTrack(11, 'D')
    self.CheckTrack(12, 'E')
    self.CheckTrack(13, 'EF')
    self.CheckTrack(14, 'E')
    self.CheckTrack(15, '')
    self.CheckTrack(100, '')

  def testComplete(self):
    # These are deliberately out of order.
    self.CheckIntervals([
        {'ts': 5, 'ph': 'X', 'dur': 1, 'args': {'name': 'B'}},
        {'ts': 3, 'ph': 'X', 'dur': 4, 'args': {'name': 'A'}},
        {'ts': 10, 'ph': 'X', 'dur': 1, 'args': {'name': 'C'}},
        {'ts': 10, 'ph': 'X', 'dur': 2, 'args': {'name': 'D'}},
        {'ts': 13, 'ph': 'X', 'dur': 1, 'args': {'name': 'F'}},
        {'ts': 12, 'ph': 'X', 'dur': 3, 'args': {'name': 'E'}}])

  def testDuration(self):
    self.CheckIntervals([
        {'ts': 3, 'ph': 'B', 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'B', 'args': {'name': 'B'}},
        {'ts': 6, 'ph': 'E'},
        {'ts': 7, 'ph': 'E'},
        # Since async intervals aren't named and must be nested, we fudge the
        # beginning of D by a tenth to ensure it's consistently detected as the
        # outermost event.
        {'ts': 9.9, 'ph': 'B', 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'B', 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'E'},
        # End of D. As end times are exclusive this should not conflict with the
        # start of E.
        {'ts': 12, 'ph': 'E'},
        {'ts': 12, 'ph': 'B', 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'B', 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'E'},
        {'ts': 15, 'ph': 'E'}])

  def testBadDurationExtraBegin(self):
    self.assertRaises(devtools_monitor.DevToolsConnectionException,
                      self.CheckIntervals,
                      [{'ts': 3, 'ph': 'B'},
                       {'ts': 4, 'ph': 'B'},
                       {'ts': 5, 'ph': 'E'}])

  def testBadDurationExtraEnd(self):
    self.assertRaises(devtools_monitor.DevToolsConnectionException,
                      self.CheckIntervals,
                      [{'ts': 3, 'ph': 'B'},
                       {'ts': 4, 'ph': 'E'},
                       {'ts': 5, 'ph': 'E'}])

  def testAsync(self):
    self.CheckIntervals([
        # A, B and F have the same category/id (so that A & B nest); C-E do not.
        {'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'B'}},
        # Not indexable.
        {'ts': 4, 'ph': 'n', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 1},
        {'ts': 7, 'ph': 'e', 'cat': 'A', 'id': 1},
        {'ts': 10, 'ph': 'b', 'cat': 'B', 'id': 2, 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'b', 'cat': 'B', 'id': 3, 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'e', 'cat': 'B', 'id': 3},
        {'ts': 12, 'ph': 'e', 'cat': 'B', 'id': 2},
        {'ts': 12, 'ph': 'b', 'cat': 'A', 'id': 2, 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'e', 'cat': 'A', 'id': 1},
        {'ts': 15, 'ph': 'e', 'cat': 'A', 'id': 2}])

  def testBadAsyncIdMismatch(self):
    self.assertRaises(
        devtools_monitor.DevToolsConnectionException,
        self.CheckIntervals,
        [{'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
         {'ts': 5, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'B'}},
         {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 2},
         {'ts': 7, 'ph': 'e', 'cat': 'A', 'id': 1}])

  def testBadAsyncExtraBegin(self):
    self.assertRaises(
        devtools_monitor.DevToolsConnectionException,
        self.CheckIntervals,
        [{'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
         {'ts': 5, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'B'}},
         {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 1}])

  def testBadAsyncExtraEnd(self):
    self.assertRaises(
        devtools_monitor.DevToolsConnectionException,
        self.CheckIntervals,
        [{'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
         {'ts': 5, 'ph': 'e', 'cat': 'A', 'id': 1},
         {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 1}])

  def testObject(self):
    # A and E share ids, which is okay as their scopes are disjoint.
    self.CheckIntervals([
        {'ts': 3, 'ph': 'N', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'N', 'id': 2, 'args': {'name': 'B'}},
        {'ts': 6, 'ph': 'D', 'id': 2},
        {'ts': 6, 'ph': 'O', 'id': 2},  #  Ignored.
        {'ts': 7, 'ph': 'D', 'id': 1},
        {'ts': 10, 'ph': 'N', 'id': 3, 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'N', 'id': 4, 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'D', 'id': 4},
        {'ts': 12, 'ph': 'D', 'id': 3},
        {'ts': 12, 'ph': 'N', 'id': 1, 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'N', 'id': 5, 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'D', 'id': 5},
        {'ts': 15, 'ph': 'D', 'id': 1}])

  def testMixed(self):
    # A and E are objects, B complete, D a duration, and C and F async.
    self.CheckIntervals([
        {'ts': 3, 'ph': 'N', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'X', 'dur': 1, 'args': {'name': 'B'}},
        {'ts': 7, 'ph': 'D', 'id': 1},
        {'ts': 10, 'ph': 'B', 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'b', 'cat': 'X', 'id': 1, 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'e', 'cat': 'X', 'id': 1},
        {'ts': 12, 'ph': 'E'},
        {'ts': 12, 'ph': 'N', 'id': 1, 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'b', 'cat': 'X', 'id': 2, 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'e', 'cat': 'X', 'id': 2},
        {'ts': 15, 'ph': 'D', 'id': 1}])


if __name__ == '__main__':
  unittest.main()
