// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_DEVICE_DESCRIPTION_PARSER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_DEVICE_DESCRIPTION_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "chrome/browser/media/router/discovery/dial/parsed_dial_device_description.h"

namespace service_manager {
class Connector;
}

namespace media_router {

// SafeDialDeviceDescriptionParser parses the given device description XML file
// safely via a utility process.
class SafeDialDeviceDescriptionParser {
 public:
  enum class ParsingError {
    kNone = 0,
    kInvalidXml = 1,
    kFailedToReadUdn = 2,
    kFailedToReadFriendlyName = 3,
    kFailedToReadModelName = 4,
    kFailedToReadDeviceType = 5,
    kMissingUniqueId = 6,
    kMissingFriendlyName = 7,
    kMissingAppUrl = 8,
    kInvalidAppUrl = 9,
    kUtilityProcessError = 10,

    // Note: Add entries only immediately above this line.
    // TODO(https://crbug.com/742517): remove this enum value.
    kTotalCount = 11,
  };

  // Callback function to be invoked when utility process finishes parsing
  // some device description XML.
  // |device_description|: device description object. Empty if parsing fails.
  // |parsing_error|: error encountered while parsing DIAL device description.
  using DeviceDescriptionCallback =
      base::OnceCallback<void(ParsedDialDeviceDescription device_description,
                              ParsingError parsing_error)>;

  SafeDialDeviceDescriptionParser();
  virtual ~SafeDialDeviceDescriptionParser();

  // Starts parsing the device description in |xml_text| in a utility process.
  // If the parsing succeeds, invokes callback with a valid
  // |device_description|, otherwise invokes callback with an empty
  // |device_description| and sets parsing error to detail the failure.
  // |connector| should be a valid connector to the ServiceManager.
  // Spec for DIAL device description:
  // http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v2.0.pdf
  // Section 2.3 Device description.
  virtual void Start(service_manager::Connector* connector,
                     const std::string& xml_text,
                     DeviceDescriptionCallback callback);

 private:
  void OnXmlParsingDone(DeviceDescriptionCallback callback,
                        std::unique_ptr<base::Value> value,
                        const base::Optional<std::string>& error);

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<SafeDialDeviceDescriptionParser> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SafeDialDeviceDescriptionParser);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_DEVICE_DESCRIPTION_PARSER_H_
