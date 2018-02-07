// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WIN_UI_AUTOMATION_UTIL_H_
#define CHROME_BROWSER_WIN_UI_AUTOMATION_UTIL_H_

// Must be before <uiautomation.h>
#include <objbase.h>

#include <uiautomation.h>

#include <string>
#include <vector>

#include "base/strings/string16.h"

// Returns a cached BSTR property of |element|.
base::string16 GetCachedBstrValue(IUIAutomationElement* element,
                                  PROPERTYID property_id);

// Debug utilities.
bool GetCachedBoolValue(IUIAutomationElement* element, PROPERTYID property_id);

int32_t GetCachedInt32Value(IUIAutomationElement* element,
                            PROPERTYID property_id);

std::vector<int32_t> GetCachedInt32ArrayValue(IUIAutomationElement* element,
                                              PROPERTYID property_id);

std::string IntArrayToString(const std::vector<int32_t>& values);

const char* GetEventName(EVENTID event_id);

const char* GetControlType(long control_type);

#endif  // CHROME_BROWSER_WIN_UI_AUTOMATION_UTIL_H_
