Many APIs in ::dbus are [required to be called from the same
thread](https://crbug.com/130984). Therefore, the dbus component maintains a
SingleThreadedTaskRunner from which all calls to dbus have to be made.

On ChromeOS, this task runner should be accessed via the DBUSThreadManager.
