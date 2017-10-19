The dbus component maintains a SingleThreadedTaskRunner from which all calls to
dbus have to be made.

On ChromeOS, this task runner should be accessed via the DBUSThreadManager.
