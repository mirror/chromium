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

import urllib

from buildbot import interfaces
from buildbot import util
from buildbot.status.builder import FAILURE
from buildbot.status.mail import MailNotifier
from email.Message import Message
from email.Utils import formatdate
from twisted.internet import defer
from zope.interface import implements


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
  _TREE_CLOSER_URL = 'http://chromium-status.appspot.com'
  
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
      self.watched.append(builder)
      return self  # subscribe to this builder

  def stepFinished(self, build, step, results):
    """A step has just finished.

    'results' is the result tuple described in IBuildStepStatus.getResults().
    """
    pass

  def buildFinished(self, name, build, results):
    """A build has just finished."""
    # If we have not failed, we have nothing to do.
    if results != FAILURE:
      return

    builder = build.getBuilder()
    if not self.isInterestingBuilder(builder, name):
      return  # nothing to do

    if not self.categories_steps:
      return self.closeTree(name, build, results)

    # Now get all the steps we must check for this builder 
    steps_to_check = []
    for category in builder.category.split(self._CATEGORY_SPLITTER):
      if category in self.categories_steps:
        steps_to_check += self.categories_steps[category]
    if '' in self.categories_steps:
      steps_to_check += self.categories_steps['']

    for build_step in build.getSteps():
      for step_text in build_step.getText():
        if step_text in steps_to_check:
          (step_result, result_strings) = build_step.getResults()
          if step_result == FAILURE:
            return self.closeTree(name, build, step_text)

  def closeTree(self, name, build, step_text):
    # Check if the tree is already closed or not:
    status = urllib.urlopen(self._TREE_STATUS_URL).read()
    if status.find('0') == -1:
      # Post a request to close the tree
      message = 'Tree is closed (Automatic: "%s" on "%s")' % (step_text, name)
      params = urllib.urlencode({'message': message, "change": "Change"})
      urllib.urlopen(self._TREE_CLOSER_URL, params)
      # Send the notification email
      return self.buildMessage(name, build, step_text)

  def buildMessage(self, name, build, step_text):
    """Send an email about the tree closing.

    Don't attach the patch as MailNotifier.buildMessage do.
    """

    projectName = self.status.getProjectName()
    job_stamp = build.getSourceStamp()
    build_url = self.status.getURLForThing(build)
    waterfall_url = self.status.getBuildbotURL()

    patch_url_text = ''
    if job_stamp and job_stamp.patch:
      patch_url_text = 'Patch: %s/steps/gclient/logs/patch\n\n' % build_url

    status_text = ('Automatically closing tree for "%s" on "%s"' %
                   (step_text, name))
    res = 'failure'

    text = """Run details: %s

%sSlave history: %swaterfall?builder=%s

Build Reason: %s

--=>  %s  <=--

Buildbot waterfall: http://build.chromium.org/
""" % (build_url,
       patch_url_text,
       urllib.quote(waterfall_url, '/:'),
       urllib.quote(name),
       build.getReason(),
       status_text)
    # TODO(maruel): Add the content of the steps in the email instead of forcing
    # users to clicks a link.

    m = Message()
    m.set_payload(text)

    m['Date'] = formatdate(localtime=True)
    m['Subject'] = self.subject % {
        'result': res,
        'projectName': projectName,
        'builder': name,
        'reason': build.getReason(),
    }
    m['From'] = self.fromaddr
    if self.reply_to:
      m['Reply-To'] = self.reply_to
    # now, who is this message going to?
    dl = []
    recipients = self.extraRecipients[:]
    if self.sendToInterestedUsers and self.lookup:
      for u in build.getInterestedUsers():
        d = defer.maybeDeferred(self.lookup.getAddress, u)
        d.addCallback(recipients.append)
        dl.append(d)
    d = defer.DeferredList(dl)
    d.addCallback(self._gotRecipients, recipients, m)
    return d
