This directory contains the code for a COM server to handle native notification
activation. This code is compiled in an executable named notification_helper.exe.

This is a standalone executable that doesn't contain Chrome, and that in order
to run Chrome it launches Chrome using a certain command line.

This executable is invoked when there is toast activation from the Windows
Action Center. Once running, the process registers the NotificationActivator
class object to a COM module. This allows COM to create the object and call its
Activate() method to handle toast activation when required. Once COM finishes
the work, the NotificationActivator class is unregistered from the module. Then
the process exits.

A NotificationActivator's CLSID depends on its Chrome channel, allowing
different NotificationActivators to be created per Chrome channel.
