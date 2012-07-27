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
#include "chromeos/display/output_configurator.h"
#include "grit/ui_resources.h"
#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skia_util.h"

using std::max;
using std::min;

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <png.h>

struct dumb_bo {
  uint32_t handle;
  uint32_t size;
  void *ptr;
  int map_count;
  uint32_t pitch;
  int hot_x;
  int hot_y;
};

namespace aura {

namespace {

int g_drm_fd;

// DRM supports only 64x64 cursors as of kernel 3.4.
const int kDrmCursorWidth = 64;
const int kDrmCursorHeight = 64;

RootWindowHostDRM* GetRootWindowHostInstance(RootWindowHostDelegate* delegate) {
  static RootWindowHostDRM* g_root_window_host = NULL;

  // TODO(sque): this is a workaround because CreateDispatcher() may get called
  // first, but a RootWindowHostDelegate will not be provided.  This code allows
  // a later call to Create() to provide a delegate to an existing
  // RootWindowHostDRM.
  if (!g_root_window_host) {
    g_root_window_host =
        new RootWindowHostDRM(
            delegate,
            gfx::Rect(RootWindowHostDRM::GetNativeScreenSize()));
  } else if (g_root_window_host->delegate() == NULL && delegate != NULL) {
    g_root_window_host->set_delegate(delegate);
  }
  return g_root_window_host;
}

}  // namespace

static struct dumb_bo *dumb_bo_create(
    int fd, const unsigned width, const unsigned height, const unsigned bpp) {
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

static int dumb_bo_map(int fd, struct dumb_bo *bo) {
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
static int dumb_bo_destroy(int fd, struct dumb_bo *bo) {
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

class RootWindowHostDRM::ImageCursors {
 public:
  ImageCursors(int drm_fd)
      : scale_factor_(0.0),
        drm_fd_(drm_fd) {
  }

  void Reload(float scale_factor) {
    if (scale_factor_ == scale_factor)
      return;
    scale_factor_ = scale_factor;
    UnloadAll();
    // The cursor's hot points are defined in chromeos's
    // src/platforms/assets/cursors/*.cfg files.
    LoadImageCursor(ui::kCursorNull, IDR_AURA_CURSOR_PTR, 9, 5);
    LoadImageCursor(ui::kCursorPointer, IDR_AURA_CURSOR_PTR, 9, 5);
    LoadImageCursor(ui::kCursorNoDrop, IDR_AURA_CURSOR_NO_DROP, 9, 5);
    LoadImageCursor(ui::kCursorCopy, IDR_AURA_CURSOR_COPY, 9, 5);
    LoadImageCursor(ui::kCursorHand, IDR_AURA_CURSOR_HAND, 9, 4);
    LoadImageCursor(ui::kCursorMove, IDR_AURA_CURSOR_MOVE, 12, 12);
    LoadImageCursor(ui::kCursorNorthEastResize,
                    IDR_AURA_CURSOR_NORTH_EAST_RESIZE, 12, 11);
    LoadImageCursor(ui::kCursorSouthWestResize,
                    IDR_AURA_CURSOR_SOUTH_WEST_RESIZE, 12, 11);
    LoadImageCursor(ui::kCursorSouthEastResize,
                    IDR_AURA_CURSOR_SOUTH_EAST_RESIZE, 11, 11);
    LoadImageCursor(ui::kCursorNorthWestResize,
                    IDR_AURA_CURSOR_NORTH_WEST_RESIZE, 11, 11);
    LoadImageCursor(ui::kCursorNorthResize,
                    IDR_AURA_CURSOR_NORTH_RESIZE, 11, 10);
    LoadImageCursor(ui::kCursorSouthResize,
                    IDR_AURA_CURSOR_SOUTH_RESIZE, 11, 10);
    LoadImageCursor(ui::kCursorEastResize, IDR_AURA_CURSOR_EAST_RESIZE, 11, 11);
    LoadImageCursor(ui::kCursorWestResize, IDR_AURA_CURSOR_WEST_RESIZE, 11, 11);
    LoadImageCursor(ui::kCursorIBeam, IDR_AURA_CURSOR_IBEAM, 12, 11);

    // TODO (varunjain): add more cursors once we have assets.
  }

  ~ImageCursors() {
    UnloadAll();
  }

  void UnloadAll() {
    std::map<int, ui::Cursor>::iterator it;
    for (it = cursors_.begin(); it != cursors_.end(); ++it)
      it->second.UnrefCustomCursor();
  }

  // Returns true if we have an image resource loaded for the |native_cursor|.
  bool IsImageCursor(gfx::NativeCursor native_cursor) {
    return cursors_.find(native_cursor.native_type()) != cursors_.end();
  }

  // Gets the DRM Cursor corresponding to the |native_cursor|.
  ui::Cursor ImageCursorFromNative(gfx::NativeCursor native_cursor) {
    DCHECK(cursors_.find(native_cursor.native_type()) != cursors_.end());
    return cursors_[native_cursor.native_type()];
  }

 private:
  struct dumb_bo* SkBitmapToCursorBO(const SkBitmap* bitmap) {
    DCHECK(bitmap->config() == SkBitmap::kARGB_8888_Config);

    if (bitmap->width() == 0 || bitmap->height() == 0)
      return NULL;

    struct dumb_bo* bo =
        dumb_bo_create(drm_fd_, kDrmCursorWidth, kDrmCursorHeight, 32);
    CHECK(bo);

    int ret = dumb_bo_map(drm_fd_, bo);
    DCHECK_EQ(ret, 0);

    // Copy bitmap contents into a temporary buffer so it can be later converted
    // to 64x64.
    uint32_t* bitmap_buffer = new uint32_t[bitmap->width() * bitmap->height()];
    CHECK(bitmap_buffer) << "Could not create bitmap buffer.";

    bitmap->lockPixels();
    gfx::ConvertSkiaToRGBA(
        static_cast<const unsigned char*>(bitmap->getPixels()),
        bitmap->width() * bitmap->height(),
        reinterpret_cast<unsigned char*>(bitmap_buffer));
    bitmap->unlockPixels();

    // Crop or expand the bitmap to a DRM cursor.
    unsigned char* drm_buffer = static_cast<unsigned char*>(bo->ptr);
    memset(drm_buffer, 0, bo->size);

    for (int y = 0; y < std::min(bitmap->height(), kDrmCursorHeight); y++) {
      memcpy(&drm_buffer[y * bo->pitch],
             &bitmap_buffer[y * bitmap->width()],
             std::min(static_cast<uint32_t>(bitmap->width() * 4), bo->pitch));
    }
    delete [] bitmap_buffer;

    return bo;
  }

  ui::Cursor CreateReffedCustomDRMCursor(int id, struct dumb_bo* bo) {
    ui::Cursor cursor = id;
    cursor.SetPlatformCursor(bo);
    cursor.RefCustomCursor();
    return cursor;
  }

  // Creates a DRM Cursor from an image resource and puts it in the cursor map.
  void LoadImageCursor(int id, int resource_id, int hot_x, int hot_y) {
    const gfx::ImageSkia* image =
        ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(resource_id);
    const SkBitmap& bitmap = SkBitmap(*image);
    // We never use scaled cursor, but the following DCHECK fails
    // because the method above computes the actual scale from the image
    // size instead of obtaining from the resource data, and the some
    // of cursors are indeed not 2x size of the 2x images.
    // TODO(oshima): Fix this and enable the following DCHECK.
    // DCHECK_EQ(actual_scale, scale_factor_);
    struct dumb_bo* bo = SkBitmapToCursorBO(&bitmap);
    bo->hot_x = std::max(std::min(hot_x, bitmap.width()), 0);
    bo->hot_y = std::max(std::min(hot_y, bitmap.height()), 0);

    cursors_[id] = CreateReffedCustomDRMCursor(id, bo);
    // |bitmap| is owned by the resource bundle. So we do not need to free it.
  }

  // A map to hold all image cursors. It maps the cursor ID to the Cursor.
  std::map<int, ui::Cursor> cursors_;

  float scale_factor_;

  // DRM device handle.
  int drm_fd_;

  DISALLOW_COPY_AND_ASSIGN(ImageCursors);
};

RootWindowHostDRM::RootWindowHostDRM(RootWindowHostDelegate* delegate,
                                     const gfx::Rect& bounds)
    : delegate_(delegate),
      modifiers_(0),
      current_cursor_(ui::kCursorNull),
      cursor_position_(0, 0),
      is_cursor_visible_(true),
      bounds_(bounds) {
  xkb_ = CreateXKB();
  CHECK(xkb_);

  fd_ = GetDRMFd();
  if (fd_ < 0) {
    // Probably permissions error
    return;
  }
  display_ = gbm_create_device(fd_);
  if (display_ == NULL) {
    LOG(ERROR) << "couldn't create gbm device.";
    return;
  }

  if (!SetupKMS()) {
    return;
  }

  saved_crtc_ = drmModeGetCrtc(fd_, kms_.encoder->crtc_id);
  if (saved_crtc_ == NULL) {
    LOG(ERROR) << "failed to save old crtc.";
    return;
  }

  // Load cursors.
  image_cursors_ = scoped_ptr<ImageCursors>(new ImageCursors(fd_));
  image_cursors_->Reload(1.0);

  // Create invisible cursor.
  struct dumb_bo* bo =
      dumb_bo_create(fd_, kDrmCursorWidth, kDrmCursorHeight, 32);
  CHECK(bo);
  int ret = dumb_bo_map(fd_, bo);
  DCHECK_EQ(ret, 0);
  memset(bo->ptr, 0, bo->size);
  empty_cursor_ = ui::Cursor(ui::kCursorNone);
  empty_cursor_.SetPlatformCursor(bo);

  current_platform_cursor_ = bo;
//  drmModeMoveCursor(fd_, kms_.encoder->crtc_id, 0, 0);

  base::MessagePumpForUI::SetDefaultDispatcher(this);
}

RootWindowHostDRM::~RootWindowHostDRM() {
  base::MessagePumpForUI::SetDefaultDispatcher(NULL);

  if (saved_crtc_)
    drmModeSetCrtc(fd_, saved_crtc_->crtc_id, saved_crtc_->buffer_id,
                   saved_crtc_->x, saved_crtc_->y,
                   &kms_.connector->connector_id, 1, &saved_crtc_->mode);

  image_cursors_.reset();

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
    LOG(ERROR) << "drmModeGetResources failed.";
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
    LOG(ERROR) << "No currently active connector found.";
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

bool RootWindowHostDRM::Dispatch(const base::NativeEvent& event) {
  switch(event->type) {
  case base::EVDEV_EVENT_RELATIVE_MOTION:
    {
      int x, y;
      x = event->motion.x + cursor_position_.x();
      y = event->motion.y + cursor_position_.y();
      event->x = x = std::min(std::max(x, 0), bounds_.width() - 1);
      event->y = y = std::min(std::max(y, 0), bounds_.height() - 1);
      event->modifiers = modifiers_;
      cursor_position_= gfx::Point(x, y);
      UpdateDRMCursor();
      MouseEvent ev(event);
      if (delegate_)
        delegate_->OnHostMouseEvent(&ev);
    }
    break;
  case base::EVDEV_EVENT_SCROLL:
    {
      event->x = cursor_position_.x();
      event->y = cursor_position_.y();
      event->modifiers = modifiers_;
      ScrollEvent ev(event);
      if (delegate_)
        delegate_->OnHostScrollEvent(&ev);
    }
    break;
  case base::EVDEV_EVENT_BUTTON:
    {
      event->x = cursor_position_.x();
      event->y = cursor_position_.y();
      // We need to issue both mouse button up/down events with modifiers on and
      // set the modifier off after the button is up.
      if (event->value) {
        switch (event->code) {
        case BTN_LEFT:
            modifiers_ |= ui::EF_LEFT_MOUSE_BUTTON;
          break;
        case BTN_RIGHT:
            modifiers_ |= ui::EF_RIGHT_MOUSE_BUTTON;
          break;
        case BTN_MIDDLE:
            modifiers_ |= ui::EF_MIDDLE_MOUSE_BUTTON;
          break;
        default:
          break;
        }
      }
      event->modifiers = modifiers_;
      if (!event->value) {
        switch (event->code) {
        case BTN_LEFT:
          modifiers_ &= ~ui::EF_LEFT_MOUSE_BUTTON;
          break;
        case BTN_RIGHT:
          modifiers_ &= ~ui::EF_RIGHT_MOUSE_BUTTON;
          break;
        case BTN_MIDDLE:
          modifiers_ &= ~ui::EF_MIDDLE_MOUSE_BUTTON;
          break;
        default:
          break;
        }
      }
      MouseEvent ev(event);
      if (delegate_)
        delegate_->OnHostMouseEvent(&ev);
    }
    break;
  case base::EVDEV_EVENT_KEY:
    {
      // Keysym are used to get ui::KeyboardCode(Windows keycode) and are used
      // by the input bus. XKeysyms are not real key symbols. We need them for
      // now because we have not build our own kernel keycode-to-
      // ui::KeyboardCode mapping yet. Also, we need to get ibus using kernel
      // keycodes too.
      // PATH: kernel keycode -> XKeysym -> ui::KeyboardCode -> real chars.
      uint32_t code = event->code + xkb_->min_key_code;
      uint32_t level = 0;

      if (((modifiers_ & ui::EF_SHIFT_DOWN) ||
          (modifiers_ & ui::EF_CAPS_LOCK_DOWN)) &&
          XkbKeyGroupWidth(xkb_, code, 0) > 1)
        level = 1;

      event->key.sym = XkbKeySymEntry(xkb_, code, level, 0);
      event->modifiers = modifiers_;
      switch (event->code) {
      case KEY_LEFTCTRL:
      case KEY_RIGHTCTRL:
        if (event->value)
          modifiers_ |= ui::EF_CONTROL_DOWN;
        else
          modifiers_ &= ~ui::EF_CONTROL_DOWN;
        break;
      case KEY_LEFTSHIFT:
      case KEY_RIGHTSHIFT:
        if (event->value)
          modifiers_ |= ui::EF_SHIFT_DOWN;
        else
          modifiers_ &= ~ui::EF_SHIFT_DOWN;
        break;
      case KEY_LEFTALT:
      case KEY_RIGHTALT:
        if (event->value)
          modifiers_ |= ui::EF_ALT_DOWN;
        else
          modifiers_ &= ~ui::EF_ALT_DOWN;
        break;
      case KEY_CAPSLOCK:
        if (event->value)
          modifiers_ ^= ui::EF_CAPS_LOCK_DOWN;
        break;
      default:
        break;
      }
      KeyEvent ev(event, false);
      if (delegate_)
        delegate_->OnHostKeyEvent(&ev);
    }
    break;
  default:
    break;
  }
  return true;
}

ui::PlatformCursor RootWindowHostDRM::GetPlatformCursor(
    gfx::NativeCursor cursor) {
  ui::Cursor drm_cursor;
  if (image_cursors_->IsImageCursor(cursor))
    drm_cursor = image_cursors_->ImageCursorFromNative(cursor);
  else if (cursor == ui::kCursorCustom)
    drm_cursor = cursor;
  else if (cursor == ui::kCursorNone)
    drm_cursor = empty_cursor_;
  else
    drm_cursor = image_cursors_->ImageCursorFromNative(ui::kCursorNull);

  struct dumb_bo* cursor_bo = drm_cursor.platform();
  if (!cursor_bo)
    LOG(ERROR) << "Invalid cursor, cursors may not have been loaded.";
  return cursor_bo;
}

void RootWindowHostDRM::SetCursorInternal(gfx::NativeCursor cursor) {
  struct dumb_bo* cursor_bo = GetPlatformCursor(cursor);
  CHECK(cursor_bo);
  int ret = drmModeSetCursor(fd_, kms_.encoder->crtc_id, cursor_bo->handle,
                             kDrmCursorWidth, kDrmCursorHeight);
  if (ret)
    LOG(ERROR) << "Unable to set cursor: " << ret;
}

void RootWindowHostDRM::UpdateDRMCursor() {
  struct dumb_bo* cursor_bo = current_platform_cursor_;
  CHECK(cursor_bo);
  int drm_x = cursor_position_.x() - cursor_bo->hot_x;
  int drm_y = cursor_position_.y() - cursor_bo->hot_y;
  drmModeMoveCursor(fd_, kms_.encoder->crtc_id, drm_x, drm_y);
}

RootWindow* RootWindowHostDRM::GetRootWindow() {
  return delegate_->AsRootWindow();
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
  return kms_.encoder->crtc_id;
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
  delegate_->OnHostResized(size);
}

gfx::Point RootWindowHostDRM::GetLocationOnNativeScreen() const {
  return bounds_.origin();
}

void RootWindowHostDRM::SetCursor(gfx::NativeCursor cursor) {
  if (cursor == ui::kCursorNone && is_cursor_visible_) {
    current_cursor_ = cursor;
    current_platform_cursor_ = GetPlatformCursor(cursor);
    ShowCursor(false);
    return;
  }

  if (current_cursor_ == cursor)
    return;
  current_cursor_ = cursor;
  current_platform_cursor_ = GetPlatformCursor(cursor);
  // Custom web cursors are handled directly.
  if (cursor == ui::kCursorCustom)
    return;
  if (is_cursor_visible_)
    SetCursorInternal(cursor);
}

void RootWindowHostDRM::ShowCursor(bool show) {
  if (show == is_cursor_visible_)
    return;

  is_cursor_visible_ = show;
  if (is_cursor_visible_)
    SetCursorInternal(show ? current_cursor_ : ui::kCursorNone);
}

bool RootWindowHostDRM::QueryMouseLocation(gfx::Point* point) {
  *point = cursor_position_;
  return true;
}

void RootWindowHostDRM::MoveCursorTo(const gfx::Point& location) {
  cursor_position_ = location;
  UpdateDRMCursor();
}

void RootWindowHostDRM::PostNativeEvent(
    const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
}

void RootWindowHostDRM::OnDeviceScaleFactorChanged(float device_scale_factor) {
  image_cursors_->Reload(device_scale_factor);
}

void RootWindowHostDRM::Show() {
  image_cursors_->Reload(delegate_->GetDeviceScaleFactor());
}

// static
int RootWindowHostDRM::GetDRMFd() {
  if (!g_drm_fd)
    g_drm_fd = open("/dev/dri/card0", O_RDWR);
  return g_drm_fd;
}

// static
void RootWindowHostDRM::SetDRMFd(int fd) {
  g_drm_fd = fd;
}

// static
RootWindowHost* RootWindowHost::Create(RootWindowHostDelegate* delegate,
                                       const gfx::Rect& bounds) {
  return GetRootWindowHostInstance(delegate);
}

// static
gfx::Size RootWindowHost::GetNativeScreenSize() {
  return gfx::Size(1280, 800);
}

MessageLoop::Dispatcher* CreateDispatcher() {
  return GetRootWindowHostInstance(NULL);
}

}  // namespace aura
