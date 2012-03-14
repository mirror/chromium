// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/root_window_host_drm.h"

#include <linux/input.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <gbm.h>
#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XKBcommon.h>

#include <algorithm>

#include "base/logging.h"
#include "base/message_loop.h"
#include "ui/aura/cursor.h"
#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/gfx/compositor/layer.h"

using std::max;
using std::min;

struct dumb_bo {
    uint32_t handle;
    uint32_t size;
    void *ptr;
    int map_count;
    uint32_t pitch;
};

namespace aura {

static struct dumb_bo *dumb_bo_create(int fd,
			  const unsigned width, const unsigned height,
			  const unsigned bpp)
{
	struct drm_mode_create_dumb arg;
	struct dumb_bo *bo;
	int ret;

	bo = new dumb_bo();
	if (!bo)
		return NULL;

	memset(&arg, 0, sizeof(arg));
	arg.width = width;
	arg.height = height;
	arg.bpp = bpp;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
	if (ret)
		goto err_free;

	bo->handle = arg.handle;
	bo->size = arg.size;
	bo->pitch = arg.pitch;

	return bo;
 err_free:
	free(bo);
	return NULL;
}

static int dumb_bo_map(int fd, struct dumb_bo *bo)
{
	struct drm_mode_map_dumb arg;
	int ret;
	void *map;

	if (bo->ptr) {
		bo->map_count++;
		return 0;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->handle;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
	if (ret)
		return ret;

	map = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   fd, arg.offset);
	if (map == MAP_FAILED)
		return -errno;

	bo->ptr = map;
	return 0;
}

#if 0
static int dumb_bo_destroy(int fd, struct dumb_bo *bo)
{
	struct drm_mode_destroy_dumb arg;
	int ret;

	if (bo->ptr) {
		munmap(bo->ptr, bo->size);
		bo->ptr = NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->handle;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
	if (ret)
		return -errno;

	free(bo);
	return 0;
}
#endif

namespace {

static const char device_name[] = "/dev/dri/card0";

}  // namespace

RootWindowHostDRM* g_root_window_host;

RootWindowHostDRM::RootWindowHostDRM(const gfx::Rect& bounds)
    : root_window_(NULL),
      modifiers_(0),
      current_cursor_(aura::kCursorNull),
      cursor_position_(0, 0),
      is_cursor_visible_(true),
      bounds_(bounds) {
  struct dumb_bo *bo;
  uint32_t *ptr;
  int ret;

  xkb_ = CreateXKB();
  CHECK(xkb_);

  fd_ = open(device_name, O_RDWR);
  if (fd_ < 0) {
    /* Probably permissions error */
    return;
  }
  display_ = gbm_create_device(fd_);
  if (display_ == NULL) {
    fprintf(stderr, "couldn't create gbm device\n");
    return;
  }

  if (!SetupKMS()) {
    return;
  }

  saved_crtc_ = drmModeGetCrtc(fd_, kms_.encoder->crtc_id);
  if (saved_crtc_ == NULL) {
    fprintf(stderr, "failed to save old crtc\n");
    return;
  }

  bo = dumb_bo_create(fd_, 64, 64, 32);
  DCHECK_NE(bo, (struct dumb_bo *)NULL);

  ret = dumb_bo_map(fd_, bo);
  DCHECK_EQ(ret, 0);
  (void)ret; // Prevent warning in release mode.

  ptr = (uint32_t *)(bo->ptr);

  /* make the cursor a white square */
  for (int i = 0; i < 64 * 64; i++)
    ptr[i] = 0xFFFFFFFF;

  ret = drmModeSetCursor(fd_, kms_.encoder->crtc_id, bo->handle, 64, 64);
  drmModeMoveCursor(fd_, kms_.encoder->crtc_id, 0, 0);

  base::MessagePumpForUI::SetDefaultDispatcher(this);
}

RootWindowHostDRM::~RootWindowHostDRM() {
  base::MessagePumpForUI::SetDefaultDispatcher(NULL);

  if (saved_crtc_)
    drmModeSetCrtc(fd_, saved_crtc_->crtc_id, saved_crtc_->buffer_id,
                   saved_crtc_->x, saved_crtc_->y,
                   &kms_.connector->connector_id, 1, &saved_crtc_->mode);

  if (display_)
    gbm_device_destroy(display_);
  if (fd_ >= 0)
    close(fd_);
}

bool RootWindowHostDRM::SetupKMS() {
  drmModeRes *resources;
  drmModeConnector *connector = NULL;
  drmModeEncoder *encoder = NULL;
  int i;

  resources = drmModeGetResources(fd_);
  if (!resources) {
    fprintf(stderr, "drmModeGetResources failed\n");
    return false;
  }

  for (i = 0; i < resources->count_connectors; i++) {
    connector = drmModeGetConnector(fd_, resources->connectors[i]);
    if (connector == NULL)
      continue;
    if (connector->connection == DRM_MODE_CONNECTED &&
	connector->count_modes > 0)
      break;
    drmModeFreeConnector(connector);
  }

  if (i == resources->count_connectors) {
    fprintf(stderr, "No currently active connector found.\n");
    return false;
  }

  for (i = 0; i < resources->count_encoders; i++) {
    encoder = drmModeGetEncoder(fd_, resources->encoders[i]);
    if (encoder == NULL)
      continue;
    if (encoder->encoder_id == connector->encoder_id)
      break;
    drmModeFreeEncoder(encoder);
  }

  kms_.connector = connector;
  kms_.encoder = encoder;
  kms_.mode = &connector->modes[0];

  return true;
}

xkb_desc * RootWindowHostDRM::CreateXKB() {
  struct xkb_rule_names names;

  names.rules = "evdev";
  names.model = "pc105";
  names.layout = "us";
  names.variant = "";
  names.options = "";

  return xkb_compile_keymap_from_rules(&names);
}

base::MessagePumpDispatcher::DispatchStatus RootWindowHostDRM::Dispatch(
    base::NativeEvent event) {
  bool handled = false;
  int modifier = 0;

  switch(event->type) {
  case base::EVDEV_EVENT_RELATIVE_MOTION:
    {
      int x, y;
      x = event->motion.x + cursor_position_.x();
      y = event->motion.y + cursor_position_.y();
      event->x = x = std::min(std::max(x, 0), bounds_.width() - 1);
      event->y = y = std::min(std::max(y, 0), bounds_.height() - 1);
      cursor_position_= gfx::Point(x, y);
      drmModeMoveCursor(fd_, kms_.encoder->crtc_id, x, y);
      event->modifiers = modifiers_;
      MouseEvent ev(event);
      handled = root_window_->DispatchMouseEvent(&ev);
    }
    break;
  case base::EVDEV_EVENT_BUTTON:
    {
      switch (event->code) {
      case BTN_LEFT:
	modifier |= ui::EF_LEFT_MOUSE_BUTTON;
	break;
      case BTN_MIDDLE:
	modifier |= ui::EF_MIDDLE_MOUSE_BUTTON;
	break;
      case BTN_RIGHT:
	modifier |= ui::EF_RIGHT_MOUSE_BUTTON;
	break;
      default:
	NOTREACHED();
	break;
      }
      if (event->value)
	modifiers_ |= modifier;
      else
	modifiers_ &= modifier;
      event->modifiers = modifiers_;
      event->x = cursor_position_.x();
      event->y = cursor_position_.y();
      MouseEvent ev(event);
      handled = root_window_->DispatchMouseEvent(&ev);
    }
    break;
  case base::EVDEV_EVENT_KEY:
    {
      uint32_t code = event->code + xkb_->min_key_code;
      uint32_t sym, level = 0;

      if ((modifiers_ & ui::EF_SHIFT_DOWN) &&
	  XkbKeyGroupWidth(xkb_, code, 0) > 1)
	level = 1;

      sym = XkbKeySymEntry(xkb_, code, level, 0);

      switch(xkb_->map->modmap[code]) {
      case XKB_COMMON_SHIFT_MASK:
	modifier |= ui::EF_SHIFT_DOWN;
	break;
      case XKB_COMMON_LOCK_MASK:
	modifier |= ui::EF_CAPS_LOCK_DOWN;
	break;
      case XKB_COMMON_CONTROL_MASK:
	modifier |= ui::EF_CONTROL_DOWN;
	break;
      case XKB_COMMON_MOD1_MASK:
	modifier |= ui::EF_ALT_DOWN;
	break;
      default:
	break;
      }
      if (event->value)
	modifiers_ |= modifier;
      else
	modifiers_ &= modifier;
      event->modifiers = modifiers_;
      event->code = sym;
      KeyEvent ev(event, false);
      handled = root_window_->DispatchKeyEvent(&ev);
    }
    break;
  default:
    break;
  }
  return handled ? EVENT_PROCESSED : EVENT_IGNORED;
}

void RootWindowHostDRM::SetRootWindow(RootWindow* root_window) {
  root_window_ = root_window;
}

gfx::Rect RootWindowHostDRM::GetBounds() const {
  return bounds_;
}

void RootWindowHostDRM::SetBounds(const gfx::Rect& bounds) {
  if (bounds_ != bounds)
    CHECK(false);
  return;
}

gfx::AcceleratedWidget RootWindowHostDRM::GetAcceleratedWidget() {
  return &kms_;
}

gfx::Size RootWindowHostDRM::GetSize() const {
  return bounds_.size();
}

void RootWindowHostDRM::SetSize(const gfx::Size& size) {
  if (size == bounds_.size())
    return;

  // Assume that the resize will go through as requested, which should be the
  // case if we're running without a window manager.  If there's a window
  // manager, it can modify or ignore the request, but (per ICCCM) we'll get a
  // (possibly synthetic) ConfigureNotify about the actual size and correct
  // |bounds_| later.
  bounds_.set_size(size);
  root_window_->OnHostResized(size);
}

gfx::Point RootWindowHostDRM::GetLocationOnNativeScreen() const {
  return bounds_.origin();
}

void RootWindowHostDRM::SetCursor(gfx::NativeCursor cursor) {
  if (cursor == kCursorNone && is_cursor_visible_) {
    current_cursor_ = cursor;
    ShowCursor(false);
    return;
  }

  if (current_cursor_ == cursor)
    return;
  current_cursor_ = cursor;
  // Custom web cursors are handled directly.
  if (cursor == kCursorCustom)
    return;
}

void RootWindowHostDRM::ShowCursor(bool show) {
  if (show == is_cursor_visible_)
    return;

  is_cursor_visible_ = show;
}

gfx::Point RootWindowHostDRM::QueryMouseLocation() {
  NOTIMPLEMENTED();
  return gfx::Point(0, 0);
}

bool RootWindowHostDRM::ConfineCursorToRootWindow() {
  NOTIMPLEMENTED();
  return true;
}

void RootWindowHostDRM::UnConfineCursor() {
  NOTIMPLEMENTED();

}

void RootWindowHostDRM::MoveCursorTo(const gfx::Point& location) {
  NOTIMPLEMENTED();
}

void RootWindowHostDRM::PostNativeEvent(
    const base::NativeEvent& native_event) {
}

struct gbm_device* RootWindowHostDRM::GetDisplay() {
  return display_;
}

// static
RootWindowHostDRM* RootWindowHostDRM::GetInstance() {
  if (!g_root_window_host) {
    g_root_window_host =
      new RootWindowHostDRM(gfx::Rect(GetNativeScreenSize()));
  }
  return g_root_window_host;
}

// static
RootWindowHost* RootWindowHost::Create(const gfx::Rect& bounds) {
  return RootWindowHostDRM::GetInstance();
}

// static
gfx::Size RootWindowHost::GetNativeScreenSize() {
  return gfx::Size(1280, 800);
}

Dispatcher* CreateDispatcher() {
  return RootWindowHostDRM::GetInstance();
}

}  // namespace aura
