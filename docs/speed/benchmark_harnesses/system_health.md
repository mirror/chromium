# System Health tests

[TOC]

## Overview

The Chrome System Health benchmarking effort aims to create a common set of user stories on the web that can be used for all Chrome speed projects. Our benchmarks mimic average web users’ activities and cover all major web platform APIs & browser features.

The web is vast, the possible combination of user activities is endless. Hence, to get a useful benchmarking tool for engineers to use for preventing regressions, launches and day to day work, we use data analysis and work with teams within Chrome to create a limited stories set that can fit a time budget of 90 minutes machine time.

These are our key cover areas for the browser:
Different user gestures: swipe, fling, text input, scroll & infinite scroll.
Video
Audio
Flash
Graphics: css, svg, canvas, webGL
Background tabs
Multi-tab switching
Back button
Follow a link
Restore tabs
Reload a page
... ([Full tracking sheet](https://docs.google.com/spreadsheets/d/1t15Ya5ssYBeXAZhHm3RJqfwBRpgWsxoib8_kwQEHMwI/edit#gid=0))

Success to us means system health benchmarks create a wide enough net that catch
major regressions before they make it to the users. This also means performance
improvement to system health benchmarks translates to actual wins on the web,
enabling teams to use this benchmark for tracking progress with the confidence
that their improvement on the suite matters to real users.

To achieve this goal, just simulating user’s activities on the web is not
enough. We also partner with
(chrome-speed-metrics)[https://groups.google.com/a/chromium.org/forum/#!forum/progressive-web-metrics]
team to track key user metrics on our user stories.


## Where are the System Health stories?

All the system health stories are located in
[tools/perf/page_sets/system_health/](../../../../tools/perf/page_sets/system_health/).

There are few classes of stories:
1. [Accessibility stories](../../../../tools/perf/page_sets/system_health/accessibility_stories.py)
2. [Background stories](../../../../tools/perf/page_sets/system_health/background_stories.py)
2. [Browsing stories](../../../../tools/perf/page_sets/system_health/browsing_stories.py)
3. [Chrome stories](../../../../tools/perf/page_sets/system_health/chrome_stories.py)
3. [Loading stories](../../../../tools/perf/page_sets/system_health/loading_stories.py)
3. [Multi-tab stories](../../../../tools/perf/page_sets/system_health/multi_tab_stories.py)
3. [Media stories](../../../../tools/perf/page_sets/system_health/media_stories.py)

## What is the structure of a SH story?
A System Health story is a subclass of
[SystemHealthStory](https://cs.chromium.org/chromium/src/tools/perf/page_sets/system_health/system_health_story.py?l=44&rcl=d5f1f0821489a8311dc437fc6b70ac0b0d72b28b):
```
class NewSystemHealthStory(SystemHealthStory):
  NAME = 'case:group:starting_url:2018'
  URL = 'https://the.starting.url'
  TAGS = [story_tags.JAVASCRIPT_HEAVY, story_tags.INFINITE_SCROLL]
  SUPPORTED_PLATFORMS = platforms.ALL_PLATFORMS  # Default.
                     or platforms.DESKTOP_ONLY
                     or platforms.MOBILE_ONLY
                     or platforms.NO_PLATFORMS

  def _Login(self, action_runner):
    # Optional. Called before the starting URL is loaded.

  def _DidLoadDocument(self, action_runner):
    # Optional. Called after the starting URL is loaded
    # (before potentially measuring memory).
```

The name must have the following structure:
1.  **Case** (load, browse, search, …). User action/journey that the story
    simulates (verb). Stories for each case are currently kept in a separate file.
    Benchmarks using the SH story set can specify which cases they want to
    include (see SystemHealthStorySet).
2.  **Group** (social, news, tools, …). General category to which the page
    (item 3) belongs.
3.  **Page** (google, facebook, nytimes, …). The name of the individual page. In
    case there are multi pages, one can use the general grouping name like
    "top_pages", or "typical_pages".
4.  **Year** (2017, 2018, 2018_q3, ...). The year (and quarter if needed for
    disambiguating) when the page is added. Note: this rule was added later,
    so the old SH stories do not have this field.

In addition, each story also has accompanied tags that define its unique
characteristic. Tags is used as the way to track coverage of system health
stories, so they should be as detailed as needed to distinct each SH story from
the others.

## How are SH stories executed?
Given a SH story set with N stories, each story is executed sequentially as
follows:

1.  Launch the browser
2.  Start tracing
3.  Run story._Login (no-op by default)
4.  Load story.URL
5.  Run story._DidLoadDocument (no-op by default)
6.  Measure memory (disabled by default)
7.  Stop tracing
8.  Tear down the browser

All the benchmarks using SH stories tear down the browser after single story.
This ensures that every story is completely independent and modifications to the
SH story set won’t cause as many regressions/improvements on the perf dashboard.

## Should I add a new SH story and how?


