#ifndef GpuMemoryBufferImageCopy_h
#define GpuMemoryBufferImageCopy_h

#include <memory>
#include "gpu/command_buffer/client/gles2_interface.h"
#include "platform/PlatformExport.h"
#include "platform/wtf/RefPtr.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace blink {

class Image;

class PLATFORM_EXPORT GpuMemoryBufferImageCopy {
 public:
  GpuMemoryBufferImageCopy(gpu::gles2::GLES2Interface*);
  ~GpuMemoryBufferImageCopy();

  gfx::GpuMemoryBuffer* CopyImage(scoped_refptr<Image>);

 private:
  bool EnsureMemoryBuffer(int width, int height);
  void OnContextLost();
  void OnContextError(const char* msg, int32_t id);

  std::unique_ptr<gfx::GpuMemoryBuffer> m_currentBuffer;

  int lastWidth_ = 0;
  int lastHeight_ = 0;
  GLuint drawFrameBuffer_;
  gpu::gles2::GLES2Interface* gl_;
  std::unique_ptr<gfx::GpuMemoryBuffer> gpuMemoryBuffer_;

  // TODO(billorr): Add error handling for context loss or GL errors before we
  // enable this by default.
};

}  // namespace blink

#endif  // GpuMemoryBufferImageCopy_h
