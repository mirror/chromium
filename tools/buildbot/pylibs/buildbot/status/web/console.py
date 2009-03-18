from __future__ import generators

import sys, time, os.path
import operator
import urllib

from buildbot import util
from buildbot import version
from buildbot.status.builder import FAILURE
from buildbot.status.web.base import HtmlResource
from buildbot.status.web import console_resources as res

def getColorClass(color, prevColor, inProgress):
  if inProgress:
      return "running"

  if not color:
      return "notstarted"

  if color == "green":
      return "success"

  if color == "red":
      if not prevColor:
          return "failure"

      if prevColor != "red":
          return "failure"
      else:
          return "warnings"

  # Any other color? Like purple?
  return "exception"

cachedBoxes = dict()

class ANYBRANCH: pass # a flag value, used below

class CachedStatusBox:
    color = None
    title = None
    details = None
    url = None
    tag = None

    def __init__(self, color, title, details, url, tag):
        self.color = color
        self.title = title
        self.details = details
        self.url = url
        self.tag = tag


class CacheStatus:
    allBoxes = dict()
    lastRevisions = dict()

    def display(self):
        data = ""
        for builder in self.allBoxes:
            lastRevision = -1
            try:
                lastRevision = self.lastRevisions[builder]
            except:
                pass
            data += "<br> %s is up to revision %d" % (builder, lastRevision)
            for revision in self.allBoxes[builder]:
               data += "<br>%s %s %s" % (builder, revision,
                                         self.allBoxes[builder][revision].color)
        return data

    def insert(self, builderName, revision, color, title, details, url, tag):
        box = CachedStatusBox(color, title, details, url, tag)
        try:
            test = self.allBoxes[builderName]
        except:
            self.allBoxes[builderName] = dict()

        self.allBoxes[builderName][revision] = box

    def get(self, builderName, revision):
        try:
            return self.allBoxes[builderName][revision]
        except:
            return None

    def trim(self):
        for builder in self.allBoxes:
          allRevs = []
          for revision in self.allBoxes[builder]:
            allRevs.append(revision)

          if len(allRevs) > 150:
             allRevs.sort()
             deleteCount = len(allRevs) - 150
             for i in range(0, deleteCount):
               del self.allBoxes[builder][allRevs[i]]

    def update(self, builderName, lastRevision):
        currentRevision = 0
        try:
            currentRevision = self.lastRevisions[builderName]
        except:
            pass

        if currentRevision < lastRevision:
            self.lastRevisions[builderName] = lastRevision

    def getRevision(self, builderName):
        try:
            return self.lastRevisions[builderName]
        except:
            return None


class TemporaryCache:
    lastRevisions = dict()

    def display(self):
        data = ""
        for builder in self.lastRevisions:
            data += "<br>%s: %s" % (builder, self.lastRevisions[builder])

        return data
            
    def insert(self, builderName, revision):
        currentRevision = 0
        try:
            currentRevision = self.lastRevisions[builderName]
        except:
            pass

        if currentRevision < revision:
            self.lastRevisions[builderName] = revision

    def updateGlobalCache(self, global_cache):
        for builder in self.lastRevisions:
            global_cache.update(builder, self.lastRevisions[builder])


class DevRevision:
    revision = None
    who = None
    comments = None
    date = None

    def __init__(self, revision, who, comments, date):
        self.revision = revision
        self.comments = comments
        self.who = who
        self.date = date


class DevBuild:
    revision = None
    color = None
    number = None
    isFinished = None
    text = None
    details = None

    def __init__(self, revision, color, number, isFinished, text, details):
        self.revision = revision
        self.color = color
        self.number = number
        self.isFinished = isFinished
        self.text = text
        self.details = details

    
class ConsoleStatusResource(HtmlResource):
    status = None
    control = None
    changemaster = None
    cache = CacheStatus()
    initialRevs = None

    def __init__(self, allowForce=True, css=None):
        HtmlResource.__init__(self)

        self.allowForce = allowForce
        self.css = css

    def getTitle(self, request):
        status = self.getStatus(request)
        projectName = status.getProjectName()
        if projectName:
            return "BuildBot: %s" % projectName
        else:
            return "BuildBot"

    def getChangemaster(self, request):
        #TODO(nsylvain): What is this?
        return request.site.buildbot_service.parent.change_svc

    def get_reload_time(self, request):
        if "reload" in request.args:
            try:
                reload_time = int(request.args["reload"][0])
                return max(reload_time, 15)
            except ValueError:
                pass
        return None

    def head(self, request):
        #TODO(nsylvain): Check how the waterfall does that.
        head = ''
        reload_time = None
        # Check if there was an arg. Don't let people reload faster than
        # every 15 seconds. 0 means no reload.
        if "reload" in request.args:
            try:
                reload_time = int(request.args["reload"][0])
                if reload_time != 0:
                  reload_time = max(reload_time, 15)
            except ValueError:
                pass

        # Sets the default reload time to 60 seconds.
        if not reload_time:
            reload_time = 60

        # Append the tag to refresh the page. 
        if reload_time is not None and reload_time != 0:
            head += '<meta http-equiv="refresh" content="%d">\n' % reload_time
        return head

    def fetchChangesFromHistory(self, status, max_depth, max_builds, debugInfo):
        allChanges = list()
        build_count = 0
        for builderName in status.getBuilderNames()[:]:
            if build_count > max_builds:
                break
            
            builder = status.getBuilder(builderName)
            build = builder.getBuild(-1)
            depth = 0
            while build and depth < max_depth and build_count < max_builds:
                depth += 1
                build_count += 1
                sourcestamp = build.getSourceStamp()
                allChanges.extend(sourcestamp.changes[:])
                build = build.getPreviousBuild()

        debugInfo["source_fetch_len"] = len(allChanges)
        return allChanges                
        
    def getAllChanges(self, source, status, debugInfo):
        allChanges = list()
        allChanges.extend(source.changes[:])

        debugInfo["source_len"] = len(source.changes)

        if len(allChanges) < 25:
            # There is not enough revisions in the source.changes. It happens
            # quite a lot because buildbot mysteriously forget about changes
            # once in a while during restart.
            # Let's try to get more changes from the builders.
            # We check the last 10 builds of all builders, and stop when we
            # are done, or have looked at 100 builds.
            if not self.initialRevs:
                self.initialRevs = self.fetchChangesFromHistory(status, 10, 100,
                                                               debugInfo)

            allChanges.extend(self.initialRevs)

            # the new changes are not sorted, and can contain duplicates.
            # Sort the list.
            allChanges.sort(key=operator.attrgetter('revision'))

            # Remove the dups
            prevChange = None
            newChanges = []
            for change in allChanges:
                rev = int(change.revision)
                if not prevChange or rev != int(prevChange.revision):
                    newChanges.append(change)
                prevChange = change
            allChanges = newChanges

        return allChanges

    def stripRevisions(self, allChanges, numRevs, branch, devName):
        revisions = []

        if not allChanges:
            return revisions

        totalRevs = len(allChanges)
        for i in range(totalRevs-1, totalRevs-numRevs, -1):
            if i < 0:
                break
            change = allChanges[i]
            if branch == ANYBRANCH or branch == change.branch:
                if not devName or change.who in devName:
                    rev = DevRevision(change.revision, change.who,
                                      change.comments, change.getTime())
                    revisions.append(rev)

        return revisions

    def getBuildDetails(self, request, builderName, build):
        details = ""
        if build.getLogs():
            for step in build.getSteps():
                (result, reason) = step.getResults()
                if result == FAILURE:
                  name = step.getName()
                  details += "<li> %s : %s. " % (builderName,
                                                 ' '.join(step.getText()))
                  if step.getLogs():
                      details += "[ "
                      for log in step.getLogs():
                          logname = log.getName()
                          logurl = request.childLink(
                              "../builders/%s/builds/%s/steps/%s/logs/%s" %
                                (urllib.quote(builderName),
                                 build.getNumber(),
                                 urllib.quote(name),
                                 urllib.quote(logname)))
                          details += "<a href=\"%s\">%s</a> " % (logurl,
                                                                 log.getName())
                      details += "]"
        return details

    def getBuildsForRevision(self, request, builder, builderName, lastRevision,
                             numBuilds, debugInfo):
        # Look at all the recent builds, and stop whenever we reached the last
        # revision-1 or numBuilds

        revision = lastRevision 
        cachedRevision = self.cache.getRevision(builderName)
        if cachedRevision and cachedRevision > lastRevision:
            revision = cachedRevision

        builds = []
        build = builder.getBuild(-1)
        number = 0
        while build and number < numBuilds:
            debugInfo["builds_scanned"] += 1
            number += 1

            # Get the last revision in this build.
            got_rev = -1
            try:
                got_rev = build.getProperty("got_revision")
            except KeyError:
                pass

            # We ignore all builds that don't have last revisions.
            # TODO(nsylvain): If the build is over, maybe it was a problem
            # with the update source step. We need to find a way to tell the
            # user that his change might have broken the source update.
            if got_rev and got_rev != -1:
                details = self.getBuildDetails(request, builderName, build)
                devBuild = DevBuild(got_rev, build.getColor(),
                                             build.getNumber(),
                                             build.isFinished(),
                                             build.getText(),
                                             details)

                builds.append(devBuild)

                # Now break if we have enough builds.
                if int(got_rev) < int(revision):
                    break

            build = build.getPreviousBuild()

        return builds

    def getAllBuildsForRevision(self, status, request, lastRevision, numBuilds,
                                categories, builders, debugInfo):
        # Dictionnary of all the builds we care about. They key is the builder
        # name.
        allBuilds = dict()

        # List of all builders in the dictionnary.
        builderList = []

        debugInfo["builds_scanned"] = 0
        # Get all the builders.
        builderNames = status.getBuilderNames()[:]
        for builderName in builderNames:
            builder = status.getBuilder(builderName)

            # Make sure we are interested in this builder.
            if categories and builder.category not in categories:
                continue

            if builders and builderName not in builders:
                continue

            # We want to display this builder.
            builderList.append(builderName)
            allBuilds[builderName] = self.getBuildsForRevision(request,
                                                               builder,
                                                               builderName,
                                                               lastRevision,
                                                               numBuilds,
                                                               debugInfo)

        return (builderList, allBuilds)

    def displayStatusLine(self, builderList, allBuilds, revision, tempCache,
                          debugInfo, subs):
        data = ""
        details = ""
        for builder in builderList:
            introducedIn = None
            firstNotIn = None

            cached_value = self.cache.get(builder, revision.revision)
            if cached_value:
                debugInfo["from_cache"] += 1
                subs["url"] = cached_value.url
                subs["title"] = cached_value.title
                subs["color"] = cached_value.color
                subs["tag"] = cached_value.tag
                data += res.main_line_status_box.substitute(subs)
                
                # If the box is red, we add the explaination in the details
                # section.
                if cached_value.details and cached_value.color == "failure":
                  details += cached_value.details

                continue


            # Find the first build that does not include the revision.
            for build in allBuilds[builder]:
                if int(build.revision) >= int(revision.revision):
                    introducedIn = build
                else:
                    firstNotIn = build
                    break

            # Get the color of the first build with the revision, and the
            # first build that does not include the revision.
            color = None
            previousColor = None
            if introducedIn:
                color = introducedIn.color
            if firstNotIn:
                previousColor = firstNotIn.color

            isRunning = False
            if introducedIn and not introducedIn.isFinished:
                isRunning = True

            url = "./waterfall"
            title = builder
            tag = ""
            current_details = None
            if introducedIn:
                current_details = introducedIn.details or ""
                url = "./builders/%s/builds/%s" % (urllib.quote(builder),
                                                   introducedIn.number)
                title += " "
                title += urllib.quote(' '.join(introducedIn.text), ' \n\\/:')

                builderStrip = builder.replace(' ', '')
                builderStrip = builderStrip.replace('(', '')
                builderStrip = builderStrip.replace(')', '')
                tag = "Tag%s%s" % (builderStrip, introducedIn.number)

            color = getColorClass(color, previousColor, isRunning)
            subs["url"] = url
            subs["title"] = title
            subs["color"] = color
            subs["tag"] = tag

            data += res.main_line_status_box.substitute(subs)

            # If the box is red, we add the explaination in the details section.
            if current_details and color == "failure":
                details += current_details

            # Add this box to the cache if it's completed so we don't have to
            # compute it again.
            if color != "running" and color != "notstarted":
              debugInfo["added_blocks"] += 1
              self.cache.insert(builder, revision.revision, color, title,
                                current_details, url, tag)
              tempCache.insert(builder, revision.revision)

        return (data, details)

    def displayPage(self, request, status, builderList, allBuilds, revisions,
                    categories, branch, tempCache, debugInfo):
        # Build the main template directory with all the informations we have.
        subs = dict()
        subs["projectUrl"] = status.getProjectURL() or ""
        subs["projectName"] = status.getProjectName() or ""
        subs["branch"] = branch or 'trunk'
        if categories:
            subs["categories"] = ' '.join(categories)
        subs["welcomeUrl"] = self.path_to_root(request) + "index.html"
        subs["version"] = version
        subs["time"] = time.strftime("%a %d %b %Y %H:%M:%S",
                                     time.localtime(util.now()))
        subs["debugInfo"] = debugInfo


        #
        # Show the header.
        #
 
        data = res.top_header.substitute(subs)
        data += res.top_info_name.substitute(subs)

        if categories:
            data += res.top_info_categories.substitute(subs)

        if branch != ANYBRANCH:
            data += res.top_info_branch.substitute(subs)

        data += res.top_info_name_end.substitute(subs)
        # Display the legend.
        data += res.top_legend.substitute(subs)

        # Display the personalize box.
        data += res.top_personalize.substitute(subs)

        data += res.top_footer.substitute(subs)


        #
        # Display the main page
        #
        data += res.main_header.substitute(subs)


        # For each revision we show one line.
        boxClass = None
        for revision in revisions:
            if not boxClass:
              boxClass = "Alt"
            else:
              boxClass = ""

            # Fill the dictionnary with these new information
            subs["alt"] = boxClass
            subs["revision"] = revision.revision
            subs["who"] = revision.who
            subs["date"] = revision.date
            comment = revision.comments or ""
            subs["comments"] = comment.replace('<', '&lt;').replace('>', '&gt;')

            # Display the revision number and the committer.
            data += res.main_line_info.substitute(subs)

            # Display the status for all builders.
            data += res.main_line_status_header.substitute(subs)

            (data_to_add, details) = self.displayStatusLine(builderList,
                                                            allBuilds,
                                                            revision,
                                                            tempCache,
                                                            debugInfo,
                                                            subs)
            data += data_to_add
            data += res.main_line_status_footer.substitute(subs)

            # Display the details of the failures, if any.
            if details:
              subs["details"] = details
              data += res.main_line_details.substitute(subs)

            # Display the comments for this revision
            data += res.main_line_comments.substitute(subs)

        data += res.main_footer.substitute(subs)

        #
        # Display the footer of the page.
        #
        debugInfo["load_time"] = time.time() - debugInfo["load_time"]
        data += res.bottom.substitute(subs)

        return data

    def body(self, request):
        "This method builds the main dev view display."

        # Debug information to display at the end of the page.
        debugInfo = dict()
        debugInfo["load_time"] = time.time()

        # get url parameters
        # Categories to show information for.
        categories = request.args.get("category", [])
        # List of all builders to show on the page.
        builders = request.args.get("builder", [])
        # Branch used to filter the changes shown.
        branch = request.args.get("branch", [ANYBRANCH])[0]
        # List of all the committers name to display on the page.
        devName = request.args.get("name", [])

        # and the data we want to render
        status = self.getStatus(request)

        projectURL = status.getProjectURL()
        projectName = status.getProjectName()

        # Get all revisions we can find.
        source = self.getChangemaster(request)
        allChanges = self.getAllChanges(source, status, debugInfo)

        debugInfo["source_all"] = len(allChanges)

        # Keep only the revisions we care about.
        # By default we process the last 50 revisions.
        # If a dev name is passed, we look for the changes by this person in the
        # last 100 revisions.
        numRevs = 50
        if devName:
          numRevs *= 2
        numBuilds = numRevs


        revisions = self.stripRevisions(allChanges, numRevs, branch, devName)
        debugInfo["revision_final"] = len(revisions)

        # Fetch all the builds for all builders until we get the next build
        # after lastRevision.
        builderList = None
        allBuilds = None
        if revisions:

            lastRevision = revisions[len(revisions)-1].revision
            debugInfo["last_revision"] = lastRevision

            (builderList, allBuilds) = self.getAllBuildsForRevision(status,
                                                request,
                                                lastRevision,
                                                numBuilds,
                                                categories,
                                                builders,
                                                debugInfo)

        tempCache = TemporaryCache()
        debugInfo["added_blocks"] = 0
        debugInfo["from_cache"] = 0

        data = ""

        if request.args.get("display_cache", None):
            data += "<br>Global Cache"
            data += self.cache.display()
            data += "<br>Temporary Cache"
            data += tempCache.display()

        data += self.displayPage(request, status, builderList, allBuilds,
                                revisions, categories, branch, tempCache,
                                debugInfo)

        if not devName and branch == ANYBRANCH and not categories:
          tempCache.updateGlobalCache(self.cache)
          self.cache.trim()

        return data