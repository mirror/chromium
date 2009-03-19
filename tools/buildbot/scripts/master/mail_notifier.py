#!/usr/bin/python
# Copyright (c) 2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A class to mail the try bot results.
"""

import urllib

from buildbot import interfaces
from buildbot import util
from buildbot.status import mail
from buildbot.status.builder import FAILURE, SUCCESS, WARNINGS
from buildbot.status.web.base import IBox
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText
from email.Utils import formatdate
from twisted.internet import defer
from zope.interface import implements


class Domain(util.ComparableMixin):
  implements(interfaces.IEmailLookup)
  compare_attrs = ["domain"]

  def __init__(self, domain):
    assert "@" not in domain
    self.domain = domain

  def getAddress(self, name):
    """If name is already an email address, pass it through."""
    if '@' in name:
      return name
    return name + "@" + self.domain


class MailNotifier(mail.MailNotifier):
  # Additional attributes that we would like to provide.
  params = ['reply_to', 'lookup']

  def __init__(self, *args, **kwargs):
    for param in self.params:
      if kwargs.has_key(param):
        setattr(self, param, kwargs.pop(param))
      else:
        setattr(self, param, None)

    if self.lookup is not None and type(self.lookup) is str:
      self.lookup = Domain(self.lookup)

    mail.MailNotifier.__init__(self, lookup=self.lookup, *args, **kwargs)

  def buildMessage(self, name, build, results):
    """Send an email about the result. Send it as a nice HTML message."""

    projectName = self.status.getProjectName()
    job_stamp = build.getSourceStamp()
    build_url = self.status.getURLForThing(build)
    waterfall_url = self.status.getBuildbotURL()
    if results == SUCCESS:
      status_text = "You are awesome! Try succeeded!"
      res = "success"
    elif results == WARNINGS:
      status_text = "Try Had Warnings"
      res = "warnings"
    else:
      status_text = "TRY FAILED"
      res = "failure"

    subject = self.subject % {
      'result': res,
      'projectName': projectName,
      'builder': name,
      'reason': build.getReason(),
      'revision': job_stamp.revision,
    }

    class DummyObject(object):
      pass
    request = DummyObject()
    request.prepath = None

    # Generate a HTML table looking like the waterfall.
    # WARNING: Gmail ignores embedded CSS style. I don't know how to fix that so
    # meanwhile, I just won't embedded the CSS style.
    html_content = (
"""<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <title>%s</title>
</head>
<body>
  <a href="%s">%s</a><p>
  %s<p>
  <a href="%s">%s</a><p>
  <table border="0" cellspacing="0">
    <tr>
    """ % (subject, waterfall_url, waterfall_url, status_text, build_url,
           build_url))

    # With a hack to fix the url root.
    html_content += IBox(build).getBox(request).td(align='center').replace(
        'href="builders/',
        'href="' + waterfall_url + 'builders/')
    html_content += '    </tr>\n'
    for step in build.getSteps():
      if step.getText():
        html_content += '    <tr>\n    '
        # With a hack to fix the url root.
        html_content += IBox(step).getBox(request).td(align='center').replace(
            'href="builders/',
            'href="' + waterfall_url + 'builders/')
        html_content += '    </tr>\n'
    html_content += """  </table>
</body>
</html>
"""

    # Simpler text content for non-html aware clients.
    text_content = """%s
%s
""" % (status_text, build_url)

    m = MIMEMultipart('alternative')
    # The HTML message, is best and preferred.
    m.attach(MIMEText(text_content, 'plain', 'iso-8859-1'))
    m.attach(MIMEText(html_content, 'html', 'iso-8859-1'))
    m['Date'] = formatdate(localtime=True)
    m['Subject'] = subject
    m['From'] = self.fromaddr
    if self.reply_to:
      m['Reply-To'] = self.reply_to
    # now, who is this message going to?
    dl = []
    recipients = self.extraRecipients[:]
    if getattr(job_stamp, 'author_emails', None):
      # Try jobs override the interested users.
      assert type(job_stamp.author_emails) is list
      print "Sending to:"
      print job_stamp.author_emails
      recipients.extend(job_stamp.author_emails)
    else:
      if self.sendToInterestedUsers and self.lookup:
        for u in build.getInterestedUsers():
          d = defer.maybeDeferred(self.lookup.getAddress, u)
          d.addCallback(recipients.append)
          dl.append(d)
    d = defer.DeferredList(dl)
    d.addCallback(self._gotRecipients, recipients, m)
    return d
