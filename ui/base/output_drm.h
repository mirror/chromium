// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _UI_BASE_OUTPUT_DRM_
#define _UI_BASE_OUTPUT_DRM_
#pragma once

#include <stdint.h>

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "ui/base/ui_export.h"

// Forward declarations from xf86drmMode.h.
struct _drmModeModeInfo;
typedef struct _drmModeModeInfo drmModeModeInfo;
struct _drmModeCrtc;
typedef struct _drmModeCrtc drmModeCrtc;
struct _drmModeEncoder;
typedef struct _drmModeEncoder drmModeEncoder;
struct _drmModeConnector;
typedef struct _drmModeConnector drmModeConnector;

// TODO(sque): Move this to chromeos/display?
namespace ui {

class UI_EXPORT ModeDRM {
 public:
  ModeDRM();
  ModeDRM(const ModeDRM& mode);
  ModeDRM(const drmModeModeInfo& mode_info);

  int width() const;
  int height() const;

  ModeDRM& operator=(const drmModeModeInfo& mode);
  ModeDRM& operator=(const ModeDRM& mode);

  bool operator==(const ModeDRM& mode);

  drmModeModeInfo& mode_info() const;

 private:
  scoped_ptr<drmModeModeInfo> mode_info_;
};

class UI_EXPORT BufferDRM {
 public:
  BufferDRM(int fd, uint32_t width, uint32_t height);
  ~BufferDRM();

  uint32_t GetBufferID() const { return buffer_id_; }
 private:
  int fd_;

  uint32_t width_;
  uint32_t height_;
  uint32_t stride_;
  uint32_t handle_;

  uint32_t buffer_id_;
};

class UI_EXPORT OutputDRM {
 public:
  OutputDRM();
  OutputDRM(int fd, int connector_id);
  ~OutputDRM();

  bool IsConnected() const;
  bool IsInternal() const;

  void GetCRTC(drmModeCrtc* info) const;
  void SetCRTC(const drmModeCrtc& info);

  ModeDRM GetPreferredMode() const;
  void GetModes(std::vector<ModeDRM>* modes) const;

  uint32_t GetID() const;
  void GetCRTCs(std::vector<drmModeCrtc>* crtcs) const;

 private:
  int fd_;
  drmModeConnector* connector_;
  drmModeEncoder* encoder_;
  drmModeCrtc* crtc_;
};

UI_EXPORT void DRMGetConnectedOutputs(int fd, std::vector<uint32_t>* outputs);
UI_EXPORT void DRMGetAllOutputs(int fd, std::vector<uint32_t>* outputs);
UI_EXPORT int DRMGetNumOutputs(int fd);
UI_EXPORT void DRMGetCRTC(int fd, uint32_t crtc_id, drmModeCrtc* crtc);

}  // namespace ui

#endif  // _UI_BASE_OUTPUT_MANAGER_DRM_
