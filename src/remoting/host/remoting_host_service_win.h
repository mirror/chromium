// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_REMOTING_HOST_SERVICE_WIN_H_
#define REMOTING_HOST_REMOTING_HOST_SERVICE_WIN_H_

#include <windows.h>

#include "base/memory/singleton.h"
#include "base/string16.h"
#include "base/synchronization/waitable_event.h"

class CommandLine;
class MessageLoop;

namespace remoting {

class HostService {
 public:
  static HostService* GetInstance();

  // This function parses the command line and selects the action routine.
  bool InitWithCommandLine(const CommandLine* command_line);

  // Invoke the choosen action routine.
  int Run();

 private:
  HostService();
  ~HostService();

  // This routine registers the service with the service control manager.
  int Install();

  // This routine uninstalls the service previously regerested by Install().
  int Remove();

  // This is a common entry point to the main service loop called by both
  // RunAsService() and RunInConsole().
  void RunMessageLoop();

  // This function handshakes with the service control manager and starts
  // the service.
  int RunAsService();

  // This function starts the service in interactive mode (i.e. as a plain
  // console application).
  int RunInConsole();

  static BOOL WINAPI ConsoleControlHandler(DWORD event);
  static VOID CALLBACK OnServiceStopped(PVOID context);

  // The control handler of the service.
  static DWORD WINAPI ServiceControlHandler(DWORD control,
                                            DWORD event_type,
                                            LPVOID event_data,
                                            LPVOID context);

  // The main service entry point.
  static VOID WINAPI ServiceMain(DWORD argc, WCHAR* argv[]);

  // The action routine to be executed.
  int (HostService::*run_routine_)();

  // The service name.
  string16 service_name_;

  // The service status structure.
  SERVICE_STATUS service_status_;

  // The service status handle.
  SERVICE_STATUS_HANDLE service_status_handle_;

  // Service message loop.
  MessageLoop* message_loop_;

  // A waitable event that is used to wait until the service is stopped.
  base::WaitableEvent stopped_event_;

  // Singleton.
  friend struct DefaultSingletonTraits<HostService>;

  DISALLOW_COPY_AND_ASSIGN(HostService);
};

}  // namespace remoting

#endif  // REMOTING_HOST_REMOTING_HOST_SERVICE_WIN_H_
