// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/output_drm.h"

#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"

namespace {

void GetOutputs(int fd, std::vector<uint32_t>* outputs, bool connected_only) {
  outputs->clear();
  drmModeRes* resources = drmModeGetResources(fd);
  CHECK(resources);
  for (int i = 0; i < resources->count_connectors; i++) {
    drmModeConnector* connector =
        drmModeGetConnector(fd, resources->connectors[i]);
    if (!connected_only || connector->encoder_id)
      outputs->push_back(connector->connector_id);
    drmModeFreeConnector(connector);
  }
  drmModeFreeResources(resources);
}

}  // namespace

namespace ui {

ModeDRM::ModeDRM() {
  mode_info_.reset(new drmModeModeInfo);
}

ModeDRM::ModeDRM(const ModeDRM& mode) {
  mode_info_.reset(new drmModeModeInfo);
  *mode_info_ = *mode.mode_info_;
}

ModeDRM::ModeDRM(const drmModeModeInfo& mode_info) {
  mode_info_.reset(new drmModeModeInfo);
  *mode_info_ = mode_info;
}

int ModeDRM::width() const {
  return mode_info_->hdisplay;
}

int ModeDRM::height() const {
  return mode_info_->vdisplay;
}

drmModeModeInfo& ModeDRM::mode_info() const {
  return *mode_info_;
}

ModeDRM& ModeDRM::operator=(const drmModeModeInfo& mode_info) {
  *mode_info_ = mode_info;
  return *this;
}

ModeDRM& ModeDRM::operator=(const ModeDRM& mode) {
  *mode_info_ = *mode.mode_info_;
  return *this;
}

bool ModeDRM::operator==(const ModeDRM& mode) {
  return (width() == mode.width()) && (height() == mode.height());
}

OutputDRM::OutputDRM() :
    fd_(-1), connector_(NULL), encoder_(NULL), crtc_(NULL) {
}

OutputDRM::OutputDRM(int fd, int connector_id) : fd_(fd),
                                                 connector_(NULL),
                                                 encoder_(NULL),
                                                 crtc_(NULL) {
  connector_ = drmModeGetConnector(fd, connector_id);
  if (connector_->encoder_id) {
    encoder_ = drmModeGetEncoder(fd, connector_->encoder_id);
    CHECK(encoder_);
  }
  if (encoder_ && encoder_->crtc_id) {
    crtc_ = drmModeGetCrtc(fd, encoder_->crtc_id);
    CHECK(crtc_);
  }

  if (IsConnected() && !(crtc_ && encoder_))
    LOG(ERROR) << "Output is connected but does not have valid encoder/CRTC.";
}

OutputDRM::~OutputDRM() {
LOG(ERROR)  << this;
  if (crtc_)
    drmModeFreeCrtc(crtc_);
  if (encoder_)
    drmModeFreeEncoder(encoder_);
  if (connector_)
    drmModeFreeConnector(connector_);
}

bool OutputDRM::IsConnected() const {
  return (connector_->connection == DRM_MODE_CONNECTED);
}

void OutputDRM::GetCRTC(drmModeCrtc* info) const {
  if (!IsConnected()) {
    memset(info, 0, sizeof(drmModeCrtc));
    return;
  }
  *info = *crtc_;
}

void OutputDRM::SetCRTC(const drmModeCrtc& info) {
  drmModeModeInfo mode = info.mode;
  if (drmModeSetCrtc(fd_, info.crtc_id, info.buffer_id, info.x, info.y,
                     &connector_->connector_id, 1, &mode));
}

ModeDRM OutputDRM::GetPreferredMode() const {
  int i;
  ModeDRM mode;
  for (i = 0; i < connector_->count_modes; i++) {
    if (connector_->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
      mode = connector_->modes[i];
      break;
    }
  }
  // If none of the connector's modes is marked as preferred, pick the first.
  if (i == connector_->count_modes)
    mode = connector_->modes[0];
  return mode;
}

void OutputDRM::GetModes(std::vector<ModeDRM>* modes) const {
  modes->clear();
  modes->resize(connector_->count_modes);
  for (int i = 0; i < connector_->count_modes; i++)
    (*modes)[i] = connector_->modes[i];
}

uint32_t OutputDRM::GetID() const {
  return connector_->connector_id;
}

bool OutputDRM::IsInternal() const {
  return (connector_->connector_type == DRM_MODE_CONNECTOR_LVDS) ||
         (connector_->connector_type == DRM_MODE_CONNECTOR_eDP);
}

void OutputDRM::GetCRTCs(std::vector<drmModeCrtc>* crtcs) const {
  crtcs->clear();
  crtcs->resize(connector_->count_encoders);
  for (int i = 0; i < connector_->count_encoders; i++) {
    drmModeEncoder* encoder = drmModeGetEncoder(fd_, connector_->encoders[i]);
    CHECK(encoder);

    drmModeCrtc* crtc = drmModeGetCrtc(fd_, encoder_->crtc_id);
    CHECK(crtc);
LOG(ERROR) << connector_;
    (*crtcs)[i] = *crtc;
LOG(ERROR) << connector_;
    drmModeFreeCrtc(crtc);
LOG(ERROR) << connector_;

    drmModeFreeEncoder(encoder);
LOG(ERROR) << connector_;
  }
}

BufferDRM::BufferDRM(int fd, uint32_t width, uint32_t height)
    : fd_(fd),
      width_(width),
      height_(height) {
  struct drm_mode_create_dumb create_arg;
  memset(&create_arg, 0, sizeof(create_arg));
  create_arg.width = width;
  create_arg.height = height;
  create_arg.bpp = 32;

  if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg))
    return;

  handle_ = create_arg.handle;
  stride_ = create_arg.pitch;

  buffer_id_ = 0;
  if (drmModeAddFB(fd_, width_, height_, 24, 32, stride_, handle_, &buffer_id_))
    LOG(ERROR) << "Could not create FB.";
}

BufferDRM::~BufferDRM() {
  if (drmModeRmFB(fd_, buffer_id_))
    LOG(ERROR) << "Could not remove FB.";

  struct drm_mode_destroy_dumb arg;
  memset(&arg, 0, sizeof(arg));
  arg.handle = handle_;
  drmIoctl(fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
}

void DRMGetConnectedOutputs(
    int fd, std::vector<uint32_t>* outputs) {
  GetOutputs(fd, outputs, true);
}

void DRMGetAllOutputs(int fd, std::vector<uint32_t>* outputs) {
  GetOutputs(fd, outputs, false);
}

int DRMGetNumOutputs(int fd) {
  drmModeRes* resources = drmModeGetResources(fd);
  CHECK(resources);
  int result = resources->count_connectors;
  drmModeFreeResources(resources);
  return result;
}

void DRMGetCRTC(int fd, uint32_t crtc_id, drmModeCrtc* crtc) {
  drmModeCrtc* crtc_temp = drmModeGetCrtc(fd, crtc_id);
  CHECK(crtc_temp);
  *crtc = *crtc_temp;
  drmModeFreeCrtc(crtc_temp);
}

}  // namespace ui
