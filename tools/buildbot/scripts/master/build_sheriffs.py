#!/usr/bin/python
# Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Retrieve the list of the current build sheriffs."""

import datetime
import urllib

class BuildSheriffs(object):
  # The CSV file to grab to retrieve the Sheriffs list.
  # Note: Modify with your @chromium.org account at:
  # http://spreadsheets.google.com/a/chromium.org/ccc?key=pYyEY4__pUJRhvu7vyCxpCw
  build_sheriff_url_ = (
    'http://spreadsheets.google.com/pub?' +
    'key=pYyEY4__pUJRhvu7vyCxpCw&output=csv&gid=0')

  # The date last time the sheriffs list was updated.
  last_check_ = None
  # Cached Sheriffs list.
  sheriffs_ = None

  @staticmethod
  def GetSheriffs():
    """Returns a list of build sheriffs for the current week."""
    # Update once per day.
    today = datetime.date.today()
    if BuildSheriffs.last_check_ != today:
      BuildSheriffs.last_check_ = today
      # Initialize in case nothing is found.
      BuildSheriffs.sheriffs_ = []

      try:
        lines = urllib.urlopen(BuildSheriffs.build_sheriff_url_).readlines()[1:]
        week = datetime.timedelta(7)
        for line in lines:
          items = line.split(',')
          # Stop as soon as a line has less than 3 elements. The rest is assumed
          # to be comments.
          if len(items) <= 2:
            break
          (month, day, year) = items[0].split('/')
          starting_date = datetime.date(int(year), int(month), int(day))
          if today >= starting_date and today < starting_date + week:
            BuildSheriffs.sheriffs_ = [item.strip() for item in items[1:]]
            break
      except ValueError:
        pass
    return BuildSheriffs.sheriffs_
