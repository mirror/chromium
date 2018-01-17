# Description of Tools for developers trying to understand memory usage

This page provides an overview of the tools available for examining memory usage
in chrome.

## Which tool should I use?

No single tool can give a full view of memory usage in Chrome. There are too
many different context involved (JS heap, DOM objects, native allocations, GPU,
etc) that any tool that collected all that information likely would not be able
to provide an actionable analysis.

Here is a table of common area of inquiry and suggested tools for examining them.

| Topic/Area of Inquiry  | Tool(s) | What this is good for | Caveats |
|----------|------------| ----------
| Object allocation over time | [`diff_heap_profiler.py`](#diff-heap-profiler) | Finding leaked C++ objects over time. |
| Self-reported stats for resource usage per subsystem  | [memory-infra in Chrome Tracing](#tracing-memory-infra) | Gaining a global overview of which subsystems are using memory. |
| Suspected Renderer DOM leaks | [Real World Leak Detector](#real-world-leak-detector) | Finding leaks within the Renderer where C++ objects are refcounted using Oilpan |
| Kernel/Drive Resource Usage | [perfmon (win), ETW](#os-tools) | Finding resource leaks that are not normally considered when the term "memory" is used. |
| Blackbox examination of process memory | [VMMAP (win)](#os-tools) | Understanding fragmentation of the memory space |

If that seems like a lot of tools and complexity, it is [but there's a reason](#no-one-true-metric).

## <a name="diff-heap-profiler"></a> `diff_heap_profiler.py`

### What this is good for
Examining allocations that occur within one point in time. This is often useful
for finding leaks as one call-stack will rise to the top as the leak is repeated
triggered.

Multiple traces can be given at once to show incremental changes.

TODO(awong): Write about options to script and the flame graph.

### Blindspots
  * Only catches allocations that pass through the allocator shim

### Instructions
  1. Ensure browser was started with OOPHP enabled and that the right processes
     are being profiled. TODO(awong): link instructions.
  2. Visit chrome://memory-internals and click "Save Dump" for a baseline.
  3. Do work that triggers events/behaviors to measure.
  2. Visit chrome://memory-internals and click "Save Dump" for a the next
     sample.
  4. Symbolize both dump files by running [`symbolize_trace.py`](../../third_party/catapult/experimental/tracing/bin/symbolize_trace.py)
  5. Run resulting traces through [`diff_heap_profiler.py`](../../third_party/catapult/experimental/tracing/bin/diff_heap_profiler.py) to show a list of new
     allocations.


## <a name="tracing-memory-infra"></a> Chrome tracing and Memory-infra
### What this is good for
Examining self-reported statistics from various subsystems on memory usages.
This is most useful for getting a high-level understanding of how memory is
distributed between the different heaps and subsystems in chrome.

It also provides a way to view heap dump allocation information collected per
process through a progressively expanding stack trace.

Though chrome://tracing itself is a timeline based plot, this data is snapshot
oriented. Thus the standard chrome://tracing plotting tools do not provide a
good means for measuring changes per snapshot.

### Blindspots
  * Statistics are self-reported via "Memory Dump Provider" interfaces. If there
    is an error in the data collection, or if there are privileged resources
    that cannot be easily measured from usermode, they will be missed.
  * The heap dump also only catches allocations that pass through the allocator shim.

### Instructions
#### Taking a trace
  1. [optional] Restart browser with OOPHP enabled at startup.
  2. Visit chrome://tracing
  3. Start a trace for memory-infra
      1. Click the "Record" button
      2. Choose "Manually select settings"
      3. [optional] Clear out all other tracing categories.
      4. Select "memory-infra" from the "Disabled by Default Categories"
      5. Click record again.
  4. Do work that triggers events/behaviors to measure.
  5. Click stop
  6. [optional] Symbolize trace
      1. Save trace file.
      2. Run trace file through [`symbolize_trace.py`](../../third_party/catapult/experimental/tracing/bin/symbolize_trace.py)
      3. Reload thet trace file

This should produce a view of the trace file with period "light" and "heavy"
memory dumps.

## <a name="global-memory-dump"></a> Global Memory Dump
### What this is good for
Many Chrome subsystems implement the
[`trace_event::MemoryDumpProvider`](../../ase/trace_event/memory_dump_provider.h)
interface to provide self-reported stats detailing their memory usage. The
Global Memory Dump view provides a snapshot-oriented view of these subsystems.

In the Analysis split screen, a single roll-up number is provided for each of
these subsystems. This can give a quick feel for where memory is allocated. The
cells can then be clicked to drill into a more detailed view of the subsystem's
stats.

### Blindspots
  * Statistics are self-reported. If the MemoryDumpProvider implemenation does
    not fully cover the resource usage of the subsystem, those resources will
    not be accounted.
  * Because the view is snapshot and not delta oriented, it is hard to see
    trends.

### Instructions
  1. Take a memory-infra trace
  2. Click on a *dark-purple* M circle.
  3. Click on a (process, subsystem) cell in `Global Memory Dump` tab within the
     Analysis View in bottom split screen.
  4. *Scroll down* to the bottom of the lower split screen to see details of
     selection (process, subsystem)

Clicking on the cell pulls up a view that lets you examine the stats
collected by the given MemoryDumpProvider however that view is often way outside
the viewport of the analysis view. Be sure to scroll down.


## <a name="heap-profile"></a> Viewing a Heap Profile
### What this is good for
Heap profiling provides extremely detailed data about object allocations and is
useful for finding memory leaks that grow over time.

Because it tracks malloc()/free() it is less useful in the Renderer process
where much of the memory allocation is handled via garbage collection in Oilpan.

### Blindspots
  * Allocations are tracked via the allocator shim. This can only catch calls
    for which the shim is effective. In Windows, this does not work on a
    component build.

### Instructions
  1. Ensure the correct heap-profiling mode is set up.
  2. Take a memory-infra trace and symbolize it.
  3. Click on a *dark-purple* M circle.
  4. Within the `Global Memory Dump` tab of the Analysis View, find the cell
     corresponding to "malloc" for the process of interest. If a heap dump was
     collected, there should be a "hotdog" menu icon.
  5. Click on the cell to inspect the malloc component.
  6. *Scroll down* to the bottom of the lower split screen. There should now
     be a "Heap details" section below the "Component details" section that
     shows a all heap allocations in a navigatable format.

Clicking on the cell pulls up a view that lets you examine the stats
collected by the given MemoryDumpProvider however that view is often way outside
the viewport of the analysis view. Be sure to scroll down.




## <a name="real-world-leak-detector"></a> Real World Leak Detector (Blink-only)

## <a name="os-tools"></a> OS Tools: perfmon, ETW, VMMAP

## <a name="no-one-true-metric"></a> No really, I want one tool/metric that views everything. Can I has it plz?
Sorry. No.

There is a natrual tradeoff between getting detailed information
and getting reliably complete information. Getting detailed information requires
instrumentation which adds complexity and selection bias to the measurement.
This reduces the reliability and completeness of the metric as code shifts over
time.

While it might be possible to instrument a specific Chrome heap
(eg, PartitionAlloc or Oilpan, or even shimming malloc()) to gather detailed
actionable data, this implicitly means the instrumentation code is making
assumptions about what process resources are used which may not be complete
or correct.

As an example of missed coverage, none of these collection methods
can notice kernel resources that are allocated (eg, GPU memory, or drive memory
such as the Windows Paged and Non-paged pools) as side effects of user mode
calls nor do they account for memory that does not go through new/malloc
(manulaly callling `mmap()`, or `VirtualAlloc()`). Querying a full view of
these allocaitons usually requires admin privileges, the semantics change
per platform, and the performance can vary from being "constant-ish" to
being dependent on virtual space size (eg, probing allocation via
VirtualQueryEx or parsing /proc/self/maps) or number of proccesses in the
system (NTQuerySystemInformation).

As an example of error in measurement, PartitionAlloc did not account for
the Windows Committed Memory model [bug](https://crbug.com/765406) leading to
a "commit leak" in Windows that was undetected in its self-reported stats.

Relying on a single metric or single tool will thus either selection bias
the data being read or not give enough detail to quickly act on problems.
