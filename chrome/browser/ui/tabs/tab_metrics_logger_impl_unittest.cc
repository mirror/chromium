// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_metrics_logger_impl.h"

#include "chrome/browser/ui/tabs/tab_metrics_event.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

using metrics::TabMetricsEvent;

// Sanity checks for TabMetricsLoggerImpl.
// See TabActivityWatcherTest for more thorough tab usage UKM tests.

// Tests that protocol schemes are mapped to the correct enumerators.
TEST(TabMetricsLoggerImplTest, Schemes) {
  TabMetricsLoggerImpl tab_metrics_logger;

  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_BITCOIN,
            tab_metrics_logger.GetSchemeValueFromString("bitcoin"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_GEO,
            tab_metrics_logger.GetSchemeValueFromString("geo"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_IM,
            tab_metrics_logger.GetSchemeValueFromString("im"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_IRC,
            tab_metrics_logger.GetSchemeValueFromString("irc"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_IRCS,
            tab_metrics_logger.GetSchemeValueFromString("ircs"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_MAGNET,
            tab_metrics_logger.GetSchemeValueFromString("magnet"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_MAILTO,
            tab_metrics_logger.GetSchemeValueFromString("mailto"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_MMS,
            tab_metrics_logger.GetSchemeValueFromString("mms"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_NEWS,
            tab_metrics_logger.GetSchemeValueFromString("news"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_NNTP,
            tab_metrics_logger.GetSchemeValueFromString("nntp"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_OPENPGP4FPR,
            tab_metrics_logger.GetSchemeValueFromString("openpgp4fpr"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_SIP,
            tab_metrics_logger.GetSchemeValueFromString("sip"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_SMS,
            tab_metrics_logger.GetSchemeValueFromString("sms"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_SMSTO,
            tab_metrics_logger.GetSchemeValueFromString("smsto"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_SSH,
            tab_metrics_logger.GetSchemeValueFromString("ssh"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_TEL,
            tab_metrics_logger.GetSchemeValueFromString("tel"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_URN,
            tab_metrics_logger.GetSchemeValueFromString("urn"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_WEBCAL,
            tab_metrics_logger.GetSchemeValueFromString("webcal"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_WTAI,
            tab_metrics_logger.GetSchemeValueFromString("wtai"));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_XMPP,
            tab_metrics_logger.GetSchemeValueFromString("xmpp"));
}

// Tests non-whitelisted protocol schemes.
TEST(TabMetricsLoggerImplTest, NonWhitelistedSchemes) {
  TabMetricsLoggerImpl tab_metrics_logger;

  // Native (non-web-based) handler.
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_OTHER,
            tab_metrics_logger.GetSchemeValueFromString("foo"));

  // Custom ("web+") protocol handlers.
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_OTHER,
            tab_metrics_logger.GetSchemeValueFromString("web+foo"));
  // "mailto" after the web+ prefix doesn't trigger any special handling.
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_OTHER,
            tab_metrics_logger.GetSchemeValueFromString("web+mailto"));

  // Nonsense protocol handlers.
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_OTHER,
            tab_metrics_logger.GetSchemeValueFromString(""));
  EXPECT_EQ(TabMetricsEvent::PROTOCOL_HANDLER_SCHEME_OTHER,
            tab_metrics_logger.GetSchemeValueFromString("mailto-xyz"));
}
