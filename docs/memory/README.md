# Memory

Landing page for all things related to memory usage in Chromium. Come to here
to find information about tools and projects related to memory usage on
Chromium.

## How is chrome's memory usage doing in the world?

Look at UMA for the Memory.\* UMAs. Confused at which to use? Start with these:


| name | description | Caveats |
|------|-------------|---------|
| Memory.\*.Committed | private, image, and file mapped memory | Windows only. Over penalizes Chrome for mapped images and files |
| Memory.Experimental.\*.<br />PrivateMemoryFootprint | New metric measuring private anonymous memory usage (swap or ram) by Chrome. | See Consistent Memory  Metrics |
| --Memory.\*.Large2-- | Measures physical memory usage. | **DO NOT USE THIS METRIC**\* |

\*Do **NOT** use `Memory.\*.Large2` as the `Large2` metrics only
count the physical ram used. This means the number varies based on the behavior
of applicatons other than Chrome making it near meaningless. Yes, they are
currently in the default finch trials. We're going to fix that.


## How do developers communicate?

Note, these channels are for developer coordination and NOT user support. If
you are a Chromium user experincing a memory related problem, file a bug
instead.

| name | description |
|------|-------------|
| [memory-dev@chromium.org]() | Discussion group for all things memory related. Post docs, discuss bugs, etc., here. |
| chrome-memory@google.com | Google internal version of the above. Use sparingly. |
| https://chromiumdev.slack.com/messages/memory/ | Slack channel for realtime discussion with memory devs. Lots of C++ sadness too. |
| crbug [Performance=Memory](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Performance%3DMemory) label | What's the distinction from `Stability=Memory`? |
| crbug [Stability=Memory](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Stability%3DMemory) label | What's the distinction from `Performance=Memory`? |


## I have a reproducable memory problem, what do I do?

Yay! Please file a [memory
bug](https://bugs.chromium.org/p/chromium/issues/entry?template=Memory%20usage).

If you are willing to do a bit more, please grab a memory infra trace and upload
that. (TODO: Add instructions for easily grabbing a trace)


## I'm a dev and I want to help. How do I get started?

Great! First, sign up for the mailing lists above and get ont he slack channel.

Second, familiarize yourself with the following:

| Topic | Description |
|-------|-------------|
| [Key Concepts in Chrome Memory](/memory/key_concepts.md) | Primer for memory terminology in Chrome. |
| [memory-infra](/memory-infra/README.md) | The primary tool used for inspecting allocations|


## What are people actively working on?
| Project | Description |
|---------|-------------|
| [Memory-Infra](/memory-infra/README.md) | Tooling and infrastructure for Memory |
| [System health benchmarks](https://docs.google.com/document/d/1pEeCnkbtrbsK3uuPA-ftbg4kzM4Bk7a2A9rhRYklmF8/edit?usp=sharing) | Automated tests based on telemetry |
| [Memory Coordinator](https://docs.google.com/document/d/1dkUXXmpJk7xBUeQM-olBpTHJ2MXamDgY_kjNrl9JXMs/edit#heading=h.swke19b7apg5) (including [Purge+Throttle/Suspend](https://docs.google.com/document/d/1EgLimgxWK5DGhptnNVbEGSvVn6Q609ZJaBkLjEPRJvI/edit)) | Centralized policy and coordination of all memory components in Chrome |


## Key knowldge areas and contacts
| Knowledge Area | Contact points |
|----------------|----------------|
| Android Process | mariahkomenko, dskiba, ssid, xunjieli |
| Browser Process | mariahkomenko, dskiba, ssid, mmenke, rsleevi, xunjieli |
| GPU | ericrk, reveman, vmiura |
| Memory metrics | erikchen, primano, ajwong, wez |
| Net Stack | xunjieli |
| Renderer Process | haraken, tasak, hajimehoshi, keishi, ericrk, hiroshige |
| V8 | hpayer, ulan, verwaest, mlippautz |


## Other docs
* [Memory Charter](https://docs.google.com/document/d/1yATy7MBclHycCUR0Jji4eczHT_ejp5lmVZOhNwNQwmM/edit#)

