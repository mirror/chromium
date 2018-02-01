# Subtle Threading Bugs and Patterns to avoid them

[Revision history](https://sites.google.com/a/chromium.org/dev/system/app/pages/admin/compare?wuid=wuid:gx:866b4baec801740&rev1=2)

[TOC]

## The Reality (Now)
base::TaskScheduler and its public API in base/task_scheduler/post_task.h make
this much harder to get wrong:

* Problems #1/2 are no longer possible: TaskRunners vended from post_task.h are
  all ref-counted.

* Problem #3 is still theoretically possible because the IO thread is shutdown
  before the TaskScheduler but PostTaskAndReply semantics are a strong part of
  the API now and should be the defacto going forward.

* Problem #4 is no longer possible: TaskScheduler is created *very* early (in
  BrowserMainLoop's constructor). From that point on TaskRunners can be created
  and tasks can be posted. They will only start running a bit later (in
  BrowserMainLoop::CreateThreads()) but this is invisible to users of the API.
  Users that need a global instance can use lazy_task_runner.h and will get a
  lazily initialized task runner of their liking one first access.

* TaskScheduler also has shutdown semantics (TaskShutdownBehavior) so it is
  easier to avoid the gotchas mentioned at the end of this document.

* For testing, the recommended approach is now to use
  base::test::ScopedTaskEnvironment which has much better support for running-
  until-idle (although we strongly encourage registering a callback to know when
  the expected event occurs rather than running-until-idle which is a very large
  hammer). content::TestBrowserThreadBundle is available as a
  ScopedTaskEnvironment that also supports the remaining BrowserThreads
  (content::RunAllTasksUntilIdle() is the run-until-idle helper in that case).

The problems outlined below still apply to the remaining BrowserThreads, but
those only being UI/IO the problem space is now highly constrained (and usage of
the problematic APIs has highly diminished over time -- especially with the
migration to TaskScheduler where many of those have even been obliterated).

## The Problem (Then)
We were using a number of patterns that were problematic:

1. Using `BrowserThread::GetMessageLoop` This isn't safe, since it could return
   a valid pointer, but since the caller isn't holding on to a lock anymore, the
   target MessageLoop could be destructed while it's being used.

1. Caching of MessageLoop pointers in order to use them later for PostTask and friends

   * This was more efficient previously (more on that later) since using
     `BrowserThread::GetMessageLoop` involved a lock.

   * But it spread logic about the order of thread destruction all over the
     code.  Code that moved from the IO thread to the file thread and back, in
     order to avoid doing disk access on the IO thread, ended up having to do an
     extra hop through the UI thread on the way back to the IO thread since the
     file thread outlives the IO thread.  Of course, most code learnt this the
     hard way, after doing the straight forward IO->file->IO thread hop and
     updating the code after seeing reliability or user crashes.

   * It made the browser shutdown fragile and hence difficult to update.

1. File thread hops using RefCountedThreadSafe objects which have non-trivial
   destructors

   * To reduce jank, frequently an object on the UI or IO thread would execute
     some code on the file thread, then post the result back to the original
     thread.  We make this easy using `base::Callback` and
     `RefCountedThreadSafe`, so this pattern happened often, but it's not always
     safe: base::Callback holds an extra reference on the object to ensure that
     it doesn't invoke a method on a deleted object.  But it's quite possible
     that before the file thread's stack unwinds and it releases the extra
     reference, that the response task on the original thread executed and
     released its own additional reference.  The file thread is then left with
     the remaining reference and the object gets destructed there.  While for
     most objects this is ok, many have non-trivial destructors, with the worst
     being ones that register with the UI-thread NotificationService.  Dangling
     pointers would be left behind and tracking these crashes from ChromeBot or
     the crash dumps has wasted several days at least for me.

1. Having browser code take different code paths if a thread didn't exist

   * This could be either deceptively harmless (i.e. execute synchronously when
     it was normally asynchronous), when in fact it makes shutdown slower
     because disk access is moved to the UI thread.

   * It could lead to data loss, if tasks are silently not posted because the
     code assumes this only happens in unit tests, when it could occur on
     browser shutdown as well.

## The Solution (Then)

* 1+2: Where possible, use `BrowserThread::PostTask`. Everywhere else, use
  `scoped_refptr<MessageLoopProxy>`.

  `BrowserThread::PostTask` and friends (i.e. `PostDelayedTask`, `DeleteSoon`,
  `ReleaseSoon`) are safe and efficient: no locks are grabbed if the target
  thread is known to outlive the current thread.  The four static methods have
  the same signature as the ones from `MessageLoop`, with the addition of the
  first parameter to indicate the target thread:

    `BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE, task);`

  Similarly, `MessageLoopProxy` has (non-static) methods with the same signature
  as the `MessageLoop` counterparts, but is safe for caching a [reference
  counting] pointer to. You can obtain a `MessageLoopProxy` via
  `Thread::message_loop_proxy()`, `BrowserThread::GetMessageLoopProxy()`, or
  `MessageLoopProxy::CreateForCurrentThread()`.

* 3: If you want to execute a method on another thread and jump back to the
  original thread, use `PostTaskAndReply()`:

    BrowserThread::PostTaskAndReply(BrowserThread::FILE, FROM_HERE,
                                    base::Bind(&Foo::DoStuffOnFileThread, this),
                                    base::Bind(&Foo::StuffDone, this));

  `PostTaskAndReply()` will make sure that both tasks are destroyed on the
  thread they were created on, so if they hold the last reference to the object,
  it will be destroyed on the originating thread. You can also use
  `PostTaskAndReplyWithResult()` to return a value from the first task and pass
  it into the second task.

  Alternatively, if your object must be destructed on a specific thread, you can
  use a trait from BrowserThread:

    class Foo : public base::RefCountedThreadSafe<Foo, BrowserThread::DeleteOnIOThread>

* 4: I've removed all the special casing and always made the objects in the
  browser code behave in one way.  If you're writing a unit test and need to use
  an object that goes to a file thread (where before it would proceed
  synchronously), you just need:

    BrowserThread file_thread(BrowserThread::FILE, MessageLoop::current());
    foo->StartAsyncTaskOnFileThread();
    MessageLoop::current()->RunAllPending();

  There are plenty of examples now in the tests.

## Gotchas (Then)
Even when using `BrowseThread` or `MessageLoopProxy`, you will still likely have
messages lost (usually resulting in memory leaks) when the target thread is in
the process of shutting down: the benefit over `MessageLoop` is primarily one of
avoiding crashing in unpredictable ways. (See this thread for debate.)

`BrowseThread` and `MessageLoopProxy::PostTask` will silently delete a task if
the thread doesn't exist.  This is done to avoid having all the code that uses
it have special cases if the target thread exists or not, and to make Valgrind
happy.  As usual, the task for `DeleteSoon/ReleaseSoon` doesn't do anything in
its destructor, so this won't cause unexpected behavior with them.  But be wary
of posted `Task` objects which have non-trivial destructors or smart pointers as
members.  I'm still on the fence about this, since while the latter is
theoretical now, it could lead to problems in the future.  I might change it so
that the tasks are not deleted when I'm ready for more Valgrind fun.

If you absolutely must know if a task was posted or not, you can check the
return value of `PostTask` and friends.  But note that even if the task was
posted successfully, there's no guarantee that it will run because the target
thread could already have a `QuitTask` queued up, or be in the early stages of
quitting.

`g_browser->io_thread()` and `file_thread()` are still around (the former for
IPC code, and the latter for Linux proxy code which is in net and so can't use
`BrowserThread`).  Don't use them unless absolutely necessary.


### More information

* https://bugs.chromium.org/p/chromium/issues/detail?id=25354

