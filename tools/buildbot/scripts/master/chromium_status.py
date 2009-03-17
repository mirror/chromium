#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Source file for buildbot.status module modifications. """

import cgi
import os
import re
import urllib

from buildbot import util
import buildbot.status.web.base as base
import buildbot.status.web.baseweb as baseweb
import buildbot.status.web.grid as grid
import buildbot.status.web.waterfall as waterfall
import buildbot.status.web.changes as changes
import buildbot.status.web.builder as builder
import buildbot.status.web.slaves as slaves
import buildbot.status.web.xmlrpc as xmlrpc
import buildbot.status.web.about as about
from buildbot.status.web.base import make_row
import buildbot.status.builder as statusbuilder
import buildbot.status.words as words
import buildbot.sourcestamp as sourcestamp

from twisted.python import log
from twisted.python import components
from twisted.web.util import Redirect
from twisted.web import html
from zope.interface import interface
from zope.interface import declarations

import chromium_config as config
import chromium_process

class BuildBox(waterfall.BuildBox):
  """Build the yellow starting-build box for a waterfall column.

  This subclass adds the builder's name to the box.  It's otherwise identical
  to the parent class, apart from naming syntax.
  """
  def getBox(self, req):
    b = self.original
    number = b.getNumber()
    url = base.path_to_build(req, b)
    reason = b.getReason()
    if reason:
      text = (('%s<br><a href="%s">Build %d</a><br>%s')
              % (b.getBuilder().getName(), url, number, html.escape(reason)))
    else:
      text = ('%s<br><a href="%s">Build %d</a>'
              % (b.getBuilder().getName(), url, number))
    color = "yellow"
    class_ = "start"
    if b.isFinished() and not b.getSteps():
      # the steps have been pruned, so there won't be any indication
      # of whether it succeeded or failed. Color the box red or green
      # to show its status
      color = b.getColor()
      class_ = base.build_get_class(b)
    return base.Box([text], color=color, class_="BuildStep " + class_)

# First unregister ICurrentBox registered by waterfall.py.
# We won't be able to unregister it without messing with ALLOW_DUPLICATES
# in twisted.python.components. Instead, work directly with adapter to
# remove the component:
origInterface = statusbuilder.BuildStatus
origInterface = declarations.implementedBy(origInterface)
registry = components.getRegistry()
registry.register([origInterface], base.IBox, '', None)
components.registerAdapter(BuildBox, statusbuilder.BuildStatus, base.IBox)

class HorizontalOneBoxPerBuilder(base.HtmlResource):
    """This shows a table with one cell per build. The color of the cell is
    the state of the most recently completed build. If there is a build in
    progress, the ETA is shown in table cell. The table cell links to the page
    for that builder. They are layed out, you guessed it, horizontally.

    builder=: show only builds for this builder. Multiple builder= arguments
              can be used to see builds from any builder in the set. If no
              builder= is given, shows them all.
    """

    def body(self, request):
      status = self.getStatus(request)
      builders = request.args.get("builder", status.getBuilderNames())

      data = "<table style='width:100%'><tr>"

      for builder_name in builders:
        builder = status.getBuilder(builder_name)
        classname = base.ITopBox(builder).getBox(request).class_
        title = builder_name

        url = (self.path_to_root(request) + "waterfall?builder=" +
               urllib.quote(builder_name, safe=''))
        link = '<a href="%s" class="%s" title="%s" \
            target=_blank> </a>' % (url, classname, title)
        data += '<td valign=bottom class=mini-box>%s</td>' % link

      data += "</tr></table>"

      return data


ANNOUNCEMENT_FILE = 'public_html/announce.html'
DEFAULT_REFRESH_TIME = 60
def GetAnnounce():
  """Creates DIV that provides visuals on tree status.
  """
  return GetStaticFileContent(ANNOUNCEMENT_FILE)

def GetStaticFileContent(file):
  if os.path.exists(file):
    announce = open(file, 'rb')
    data = announce.read().strip()
    announce.close()
    return data
  else:
    return ''
    
def GetRefresh(reload_time):
  if reload_time is None:
    reload_time = DEFAULT_REFRESH_TIME
  return '<meta http-equiv="refresh" content="%d">\n' % reload_time

class WaterfallStatusResource(waterfall.WaterfallStatusResource):
  """Class that overrides default behavior of
  waterfall.WaterfallStatusResource. """


  def head(self, request):
    """ Adds META to refresh page by specified value in the request,
    sets to one minute by default. """
    # Don't auto-refresh when last_time is set.
    if request.args.get('last_time'):
      return ""
    return GetRefresh(self.get_reload_time(request))

  def body(self, request):
    """Calls default body method and prepends the Announce file"""
    # Limit access to the X last days. Buildbot doesn't scale well.
    # TODO(maruel) Throttle the requests instead or just fix the code to cache
    # data.
    stop_gap_in_seconds = 60 * 60 * 24 * 2
    earliest_accepted_time = util.now() - stop_gap_in_seconds

    # Will throw a TypeError if last_time is not a number.
    last_time = int(request.args.get('last_time', [0])[0])
    if (last_time and
        last_time < earliest_accepted_time and
        not request.args.get('force', [False])[0]):
      return """To prevent DOS of the waterfall, heavy request like this
are blocked. If you know what you are doing, ask a Chromium Buildbot
administrator how to bypass the protection."""
    else:
      data = waterfall.WaterfallStatusResource.body(self, request)
      return "%s %s" % (GetAnnounce(), data)

class GridStatusResource(grid.GridStatusResource):
  """Class that overrides default behavior of grid.GridStatusResource. """

  def head(self, request):
    """ Adds META to refresh page by specified value in the request,
    sets to one minute by default. """
    return GetRefresh(self.get_reload_time(request))

  def body(self, request):
    """Calls default body method and prepends the announce file"""    

    data = grid.GridStatusResource.body(self, request)
    return "%s %s" % (GetAnnounce(), data)

class StatusResourceBuilder(builder.StatusResourceBuilder):
  """Class that overrides default behavior of builders.StatusResourceBuilder.

  The reason for that is to expose additional HTML controls and
  set BuildRequest custom attributes we need to add.
  """

  def body(self, req):
    """ Overriden method from builders.StatusResourceBuilder. The only
    change in original behavior is added new checkbox for clobbering."""
    b = self.builder_status
    control = self.builder_control
    status = self.getStatus(req)

    slaves = b.getSlaves()
    connected_slaves = [s for s in slaves if s.isConnected()]

    projectName = status.getProjectName()

    data = '<a href="%s">%s</a>\n' % (self.path_to_root(req), projectName)

    data += ('<h1><a href="%swaterfall?builder=%s">Builder: %s</a></h1>\n' %
             (self.path_to_root(req),
              urllib.quote(b.getName(), safe=''),
              html.escape(b.getName())))

    # the first section shows builds which are currently running, if any.

    current = b.getCurrentBuilds()
    if current:
      data += "<h2>Currently Building:</h2>\n"
      data += "<ul>\n"
      for build in current:
        data += " <li>" + self.build_line(build, req) + "</li>\n"
      data += "</ul>\n"
    else:
      data += "<h2>no current builds</h2>\n"

    # Then a section with the last 5 builds, with the most recent build
    # distinguished from the rest.

    data += "<h2>Recent Builds:</h2>\n"
    data += "<ul>\n"
    for i,build in enumerate(b.generateFinishedBuilds(num_builds=5)):
      data += " <li>" + self.make_line(req, build, False) + "</li>\n"
      if i == 0:
        data += "<br />\n" # separator
        # TODO: or empty list?
    data += "</ul>\n"


    data += "<h2>Buildslaves:</h2>\n"
    data += "<ol>\n"
    for slave in slaves:
      name = slave.getName()
      if not name:
        name = ''
      data += "<li><b>%s</b>: " % html.escape(name)
      if slave.isConnected():
        data += "CONNECTED\n"
        if slave.getAdmin():
          data += make_row("Admin:", html.escape(slave.getAdmin()))
        if slave.getHost():
          data += "<span class='label'>Host info:</span>\n"
          data += html.PRE(slave.getHost())
      else:
        data += ("NOT CONNECTED\n")
      data += "</li>\n"
    data += "</ol>\n"

    if control is not None and connected_slaves:
      forceURL = urllib.quote(req.childLink("force"))
      data += (
        """
        <form action='%(forceURL)s' class='command forcebuild'>
        <p>To force a build, fill out the following fields and
        push the 'Force Build' button</p>
        <table>
          <tr class='row' valign="bottom">
            <td>
              <span class="label">Your name<br>
               (<span style="color: red;">please fill in at least this
               much</span>):</span>
            </td>
            <td>
              <span class="field"><input type='text' name='username' /></span>
            </td>
          </tr>
          <tr class='row'>
            <td>
              <span class="label">Reason for build:</span>
            </td>
            <td>
              <span class="field"><input type='text' name='comments' /></span>
            </td>
          </tr>
          <tr class='row'>
            <td>
              <span class="label">Branch to build:</span>
            </td>
            <td>
              <span class="field"><input type='text' name='branch' /></span>
            </td>
          </tr>
          <tr class='row'>
            <td>
              <span class="label">Revision to build:</span>
            </td>
            <td>
              <span class="field"><input type='text' name='revision' /></span>
            </td>
          </tr>
          <tr class='row'>
            <td>
              <span class="label">Clobber:</span>
            </td>
            <td>
              <span class="field"><input type='checkbox' name='clobber' />
              </span>
            </td>
          </tr>
        </table>
        <input type='submit' value='Force Build' />
        </form>
        """) % {"forceURL": forceURL}
    elif control is not None:
      data += """
      <p>All buildslaves appear to be offline, so it's not possible
      to force this build to execute at this time.</p>
      """

    if control is not None:
      pingURL = urllib.quote(req.childLink("ping"))
      data += """
      <form action="%s" class='command pingbuilder'>
      <p>To ping the buildslave(s), push the 'Ping' button</p>

      <input type="submit" value="Ping Builder" />
      </form>
      """ % pingURL

    return data

  def force(self, req):
    """ Overriden method from builders.StatusResourceBuilder. The only
    change to original behavior is that it sets 'clobber' value on the
    BuildRequest."""

    name = req.args.get("username", ["<unknown>"])[0]
    reason = req.args.get("comments", ["<no reason specified>"])[0]
    branch = req.args.get("branch", [""])[0]
    revision = req.args.get("revision", [""])[0]
    clobber = req.args.get("clobber", [False])[0]

    if clobber:
      clobber_str = ' (clobbered)'
    else:
      clobber_str = ''
    r = ("The web-page 'force build' button%s was pressed by %s: %s" %
        (clobber_str, name, reason))

    message = ("web forcebuild of builder '%s', branch='%s', revision='%s'"
               ", clobber='%s'")
    log.msg(message
        % (self.builder_status.getName(), branch, revision, clobber))

    if not self.builder_control:
      # TODO: tell the web user that their request was denied
      log.msg("but builder control is disabled")
      return Redirect("..")

    # keep weird stuff out of the branch and revision strings. TODO:
    # centralize this somewhere.
    if not re.match(r'^[\w\.\-\/]*$', branch):
      log.msg("bad branch '%s'" % branch)
      return Redirect("..")
    if not re.match(r'^[\w\.\-\/]*$', revision):
      log.msg("bad revision '%s'" % revision)
      return Redirect("..")
    if branch == "":
      branch = None
    if revision == "":
      revision = None

    # TODO: if we can authenticate that a particular User pushed the
    # button, use their name instead of None, so they'll be informed of
    # the results.
    s = sourcestamp.SourceStamp(branch=branch, revision=revision)
    req = chromium_process.BuildRequest(r, s, self.builder_status.getName(),
                                      clobber=clobber)
    try:
      self.builder_control.requestBuildSoon(req)
    except interfaces.NoSlaveError:
      # TODO: tell the web user that their request could not be
      # honored
      pass
    return Redirect("../..")

class BuildersResource(builder.BuildersResource):
  """ Overrides instantiated builder.StatusResourceBuilder class with our
  custom implementation.

  Ideally it would be nice to expose builder.StatusResourceBuilder on
  WebStatus. Unfortunately BuildersResource is the one that defines
  builder.StatusResourceBuilder as its child resource.
  """
  def getChild(self, path, req):
    s = self.getStatus(req)
    if path in s.getBuilderNames():
      builder_status = s.getBuilder(path)
      builder_control = None
      c = self.getControl(req)
      if c:
        builder_control = c.getBuilder(path)
      # We return chromium_status.StatusResourceBuilder instance here. The code
      # is exactly the same and just having the code in this source file makes
      # it instantiate chromium_status.StatusResourceBuilder
      return StatusResourceBuilder(builder_status, builder_control)

    return base.HtmlResource.getChild(self, path, req)


class WebStatus(baseweb.WebStatus):
  """Class that provides hook to modify default behavior of
  baseweb.WebStatus.

  If you wish to override web status classes, this is the place
  to instantiate them.
  """

  def setupUsualPages(self):
    """ The method that defines resources to expose.

    We are supplying our modified WaterfallStatusResource and BuildersResource
    instances.
    """
    # baseweb.putChild() just records the putChild() calls and doesn't
    # actually add them to the server until initialization is complete.
    # So call the superclass to get the default pages supplied by
    # the current version of buildbot, and then just override our
    # custom ones.
    baseweb.WebStatus.setupUsualPages(self)

    self.putChild("waterfall", WaterfallStatusResource())
    self.putChild("grid", GridStatusResource())
    self.putChild("builders", BuildersResource())
    self.putChild("horizontal_one_box_per_builder",
                  HorizontalOneBoxPerBuilder())
