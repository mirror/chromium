# notification_helper EXE

This notification_helper EXE acts as a COM server to handle Windows 10 native
notification activation.

This EXE is invoked whenever there is toast activation from Windows Action
Center. Once running, it registers the NotificationActivator class object to a
COM module. This allows COM to create the object and call its Activate() method
to handle toast activation when required. Once COM finishes the work, the
NotificationActivator class is unregistered from the module. This signals a
waitable event, which notifies this EXE to exit.

We introduced different CLSIDs for the NotificationActivator class in each
Chrome channel. Therefore, each Chrome channel that has its own
notification_helper process.
