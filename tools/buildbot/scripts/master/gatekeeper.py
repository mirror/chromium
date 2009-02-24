#!/usr/bin/python2.4
#
# Copyright 2008 Google Inc. All Rights Reserved.

"""A StatusReceiver module to potentially close the tree when needed.

The GateKeeper class can be given a dictionary of categories of builder to a set
of critical steps to validate. If no steps are given for a category, we simply
check the results for FAILURES. If no categories are given, we check all
builders for FAILURES results. A list of builder exclusions can also be given so
that some specific builders could be ignored. The rest of the arguments are the
same as the MailNotifier, refer to its documentation for more details.

Since the behavior is very similar to the MainNotifier, we simply inherit from
it and also reuse some of its methods to send emails.
"""

import time
import urllib

from buildbot import interfaces
from buildbot import util
from buildbot.status.builder import FAILURE
from buildbot.status.mail import MailNotifier
from buildbot.status.web.base import IBox
from build_sheriffs import BuildSheriffs
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText
from email.Utils import formatdate
from twisted.internet import defer
from zope.interface import implements

def getAllRevisions(build):
  source_stamp = build.getSourceStamp()
  if source_stamp and source_stamp.changes:
    return [change.revision for change in source_stamp.changes]

def getLatestRevision(build):
  revisions = getAllRevisions(build)
  if revisions:
    return max(revisions)


class Domain(util.ComparableMixin):
  implements(interfaces.IEmailLookup)
  compare_attrs = ['domain']

  def __init__(self, domain):
    assert '@' not in domain
    self.domain = domain

  def getAddress(self, name):
    """If name is already an email address, pass it through."""
    if '@' in name:
      return name
    return name + '@' + self.domain


class GateKeeper(MailNotifier):
  """This is a status notifier which closes the tree upon failures."""

  _CATEGORY_SPLITTER = '|'
  _TREE_STATUS_URL = 'http://chromium-status.appspot.com/status'
  _MINIMUM_DELAY_BETWEEN_CLOSE = 600  # 10 minutes in seconds

  _last_closure_revision = 0
  _last_time_mail_sent = None
  
  # Attributes we want to set from provided arguments, or set to None
  params = ['reply_to', 'lookup', 'categories_steps', 'exclusions']

  def __init__(self, *args, **kwargs):
    """Constructor with following specific arguments (on top of base class').

    @type categories_steps: Dictionary of category string mapped to a list of
                            step strings.
    @param categories_steps: For each category name we can specify the steps we
                             want to check for success to keep the tree opened.
                             An empty list of steps means that we simply check
                             for results == FAILURE to close the tree. Defaults
                             to None for the dictionary, which means all
                             categories, and the empty string category can be
                             used to say all builders.

    @type exclusions: list of strings
    @param exclusions: A list of builder names which we want to ignore. Defaults
                       to None.
    """
    for param in self.params:
      if param in kwargs:
        setattr(self, param, kwargs.pop(param))
      else:
        setattr(self, param, None)

    if self.lookup is not None and type(self.lookup) is str:
      self.lookup = Domain(self.lookup)

    MailNotifier.__init__(self, lookup=self.lookup, *args, **kwargs)

  def isInterestingBuilder(self, builder, name):
    """Confirm if we are interested in this builder."""
    if self.exclusions and name in self.exclusions:
      return False
    if not self.categories_steps or '' in self.categories_steps:
      return True

    if not builder.category:
      return False
    for category in builder.category.split(self._CATEGORY_SPLITTER):
      if category in self.categories_steps:
        return True
    return False

  def builderAdded(self, name, builder):
    # Only subscribe to builders we are interested in.
    if self.isInterestingBuilder(builder, name):
      return self  # subscribe to this builder

  def buildStarted(self, name, build):
    """A build has started allowing us to register for stepFinished."""
    builder = build.getBuilder()
    if self.isInterestingBuilder(builder, name):
      return self

  def buildFinished(self, name, build, results):
    """Must be overloaded to avoid the base class sending email."""
    pass

  def stepFinished(self, build, step, results):
    """A build step has just finished."""
    # If we have not failed, or are not interested in this builder,
    # then we have nothing to do.
    builder = build.getBuilder()
    if results[0] != FAILURE or not self.isInterestingBuilder(builder, name):
      return

    name  = builder.getName()
    if not self.categories_steps:
      return self.closeTree(name, build, step.getText())

    # Check if the slave is still alive.
    # We should not close the tree for inactive slaves.
    slave_name = build.getSlavename()
    if slave_name in self.status.getSlaveNames():
      slave = self.status.getSlave(slave_name)
      if slave and not slave.isConnected():
        print "The slave was disconnected, not closing the tree"
        return

    # Now get all the steps we must check for this builder 
    steps_to_check = []
    for category in builder.category.split(self._CATEGORY_SPLITTER):
      if category in self.categories_steps:
        steps_to_check += self.categories_steps[category]
    if '' in self.categories_steps:
      steps_to_check += self.categories_steps['']

    for step_text in step.getText():
      if step_text in steps_to_check:
        return self.closeTree(name, build, step_text)

  def closeTree(self, name, build, step_text):
    # We don't want to do this too often
    if (self._last_time_mail_sent and self._last_time_mail_sent >
        time.time() - self._MINIMUM_DELAY_BETWEEN_CLOSE):
      return
    self._last_time_mail_sent = time.time()

    # Check if the tree is already closed or not.
    if urllib.urlopen(self._TREE_STATUS_URL).read().find('0') != -1:
      return

    # If the tree is opened, we don't want to close it again for the same
    # revision, or an earlier one in case the build that just finished is a
    # slow one and we already fixed the problem and manually opened the tree.
    latest_revision = getLatestRevision(build)
    if latest_revision:
      if latest_revision <= self._last_closure_revision:
        return
      else:
        self._last_closure_revision = latest_revision
    else:
      if not build.getResponsibleUsers():
        # If we don't have a version stamp nor a blame list, then this is most
        # likely a build started manually, and we don't want to close the
        # tree.
        return
      else:
        latest_revision = 0

    # Send the notification email.
    defered_object = self.buildMessage(name, build, step_text)

    # Post a request to close the tree.
    message = ('Tree is closed (Automatic: "%s" on "%s" from %d: %s)' %
               (step_text, name, latest_revision,
                ", ".join(build.getResponsibleUsers())))
    password_file = file(".status_password")
    params = urllib.urlencode({'message': message,
                               'username': 'buildbot@chromium.org',
                               'password': password_file.read().strip()})
    urllib.urlopen(self._TREE_STATUS_URL, params)

    return defered_object

  def buildMessage(self, name, build, step_text):
    """Send an email about the tree closing.

    Don't attach the patch as MailNotifier.buildMessage do.
    """

    projectName = self.status.getProjectName()
    revisions_list = getAllRevisions(build)
    build_url = self.status.getURLForThing(build)
    waterfall_url = self.status.getBuildbotURL()
    status_text = ('Automatically closing tree for "%s" on "%s"' %
                   (step_text, name))
    blame_list = ",".join(build.getResponsibleUsers())
    revisions_string = ''
    latest_revision = 0
    if revisions_list:
      revisions_string = ", ".join([str(rev) for rev in revisions_list])
      latest_revision = max([rev for rev in revisions_list])

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
  Revision: %s<br>
  Blame list: %s<p>

  <table border="0" cellspacing="0">
    <tr>
    """ % (status_text, waterfall_url, waterfall_url, status_text, build_url,
           build_url, revisions_string, blame_list))

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
    text_content = (
"""%s

%s

%swaterfall?builder=%s

--=>  %s  <=--

Revision: %s
Blame list: %s

Buildbot waterfall: http://build.chromium.org/
""" % (status_text,
       build_url,
       urllib.quote(waterfall_url, '/:'),
       urllib.quote(name),
       status_text,
       revisions_string,
       blame_list))

    m = MIMEMultipart('alternative')
    # The HTML message, is best and preferred.
    m.attach(MIMEText(text_content, 'plain', 'iso-8859-1'))
    m.attach(MIMEText(html_content, 'html', 'iso-8859-1'))

    m['Date'] = formatdate(localtime=True)
    m['Subject'] = self.subject % {
        'result': 'failure',
        'projectName': projectName,
        'builder': name,
        'reason': build.getReason(),
        'revision': latest_revision,
    }
    m['From'] = self.fromaddr
    if self.reply_to:
      m['Reply-To'] = self.reply_to
    # now, who is this message going to?
    dl = []
    recipients = list(self.extraRecipients[:])
    recipients.extend(BuildSheriffs.GetSheriffs())
    if self.sendToInterestedUsers and self.lookup:
      for u in build.getInterestedUsers():
        d = defer.maybeDeferred(self.lookup.getAddress, u)
        d.addCallback(recipients.append)
        dl.append(d)
    d = defer.DeferredList(dl)
    d.addCallback(self._gotRecipients, recipients, m)
    return d
