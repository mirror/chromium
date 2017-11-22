// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_browser_main_parts.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/url_constants.h"
#include "content/shell/browser/layout_test/layout_test_browser_context.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/shell/browser/shell_devtools_manager_delegate.h"
#include "content/shell/browser/shell_net_log.h"
#include "content/shell/common/shell_switches.h"
#include "media/base/mime_util.h"
#include "net/base/filename_util.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "ppapi/features/features.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#include "content/shell/browser/shell_plugin_service_filter.h"
#endif

#if defined(OS_ANDROID)
#include "components/crash/content/browser/crash_dump_manager_android.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#endif

#if defined(USE_AURA) && defined(USE_X11)
#include "ui/events/devices/x11/touch_factory_x11.h"  // nogncheck
#endif
#if !defined(OS_CHROMEOS) && defined(USE_AURA) && defined(OS_LINUX)
#include "ui/base/ime/input_method_initializer.h"
#endif

namespace content {

LayoutTestBrowserMainParts::LayoutTestBrowserMainParts(
    const MainFunctionParams& parameters)
    : ShellBrowserMainParts(parameters) {
}

LayoutTestBrowserMainParts::~LayoutTestBrowserMainParts() {
}

void LayoutTestBrowserMainParts::InitializeBrowserContexts() {
  set_browser_context(new LayoutTestBrowserContext(false, net_log()));
  set_off_the_record_browser_context(nullptr);
}

void LayoutTestBrowserMainParts::InitializeMessageLoopContext() {
#if BUILDFLAG(ENABLE_PLUGINS)
  PluginService* plugin_service = PluginService::GetInstance();
  plugin_service_filter_.reset(new ShellPluginServiceFilter);
  plugin_service->SetFilter(plugin_service_filter_.get());
#endif

// Unless/until WebM files are added to the media layout tests, we need to
// avoid removing MP4/H264/AAC so that layout tests can run on Android.
// TODO(chcunningham): We should fix the tests to always use non-proprietary
// codecs and just delete this code. http://crbug.com/787575
#if !defined(OS_ANDROID)
  media::RemoveProprietaryMediaTypesAndCodecsForTests();
#endif
}

}  // namespace
