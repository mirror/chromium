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

Success to us means system health benchmarks create a wide enough net that catch major  regressions before they make it to the users. This also means performance improvement to system health benchmarks translates to actual wins on the web, enabling teams to use this benchmark for tracking progress with the confidence that their improvement on the suite matters to real users.

To achieve this goal, just simulating user’s activities on the web is not enough. We also partner with (chrome-speed-metrics)[https://groups.google.com/a/chromium.org/forum/#!forum/progressive-web-metrics] team to track key user metrics on our user stories.


## Where are the System Health stories?

All the system health stories are located in
tools/perf/page_sets/system_health/
