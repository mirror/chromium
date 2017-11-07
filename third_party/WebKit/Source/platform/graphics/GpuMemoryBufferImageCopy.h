#ifndef GpuMemoryBufferImageCopy_h
#define GpuMemoryBufferImageCopy_h

#include <memory>
#include "platform/PlatformExport.h"
#include "platform/wtf/RefPtr.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace blink {

class WebGraphicsContext3DProvider;
class Image;

class PLATFORM_EXPORT GpuMemoryBufferImageCopy {
 public:
  GpuMemoryBufferImageCopy();

  gfx::GpuMemoryBuffer* CopyImage(RefPtr<Image>);

 private:
  bool EnsureMemoryBuffer(int width, int height);
  void OnContextLost();
  void OnContextError(const char* msg, int32_t id);

  std::unique_ptr<gfx::GpuMemoryBuffer> m_currentBuffer;

  int lastWidth_ = 0;
  int lastHeight_ = 0;
  std::unique_ptr<WebGraphicsContext3DProvider> contextProvider_;
  std::unique_ptr<gfx::GpuMemoryBuffer> gpuMemoryBuffer_;

  // TODO(billorr): Add error handling for context loss or GL errors before we
  // enable this by default.
};

}  // namespace blink

#endif  // GpuMemoryBufferImageCopy_h
