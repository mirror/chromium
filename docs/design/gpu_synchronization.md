# GPU Synchronization in Chrome

Chrome supports multiple mechanisms for sequencing GPU drawing operations, this
document provides a brief overview.

[TOC]

## References

For details about the underlying mechanisms, please refer to the respective
documentation:

* https://www.khronos.org/opengl/wiki/Synchronization
* https://www.khronos.org/registry/EGL/extensions/KHR/EGL_KHR_fence_sync.txt
* https://www.khronos.org/registry/EGL/extensions/KHR/EGL_KHR_wait_sync.txt
* https://www.khronos.org/registry/EGL/extensions/ANDROID/EGL_ANDROID_native_fence_sync.txt
* [CHROMIUM_sync_point extension](/src/gpu/GLES2/extensions/CHROMIUM/CHROMIUM_sync_point.txt)
* [CHROMIUM_fence extension](/src/gpu/GLES2/extensions/CHROMIUM/CHROMIUM_fence.txt)

## Terminology

**GL Sync Object**: Generic GL-level synchronization object that can be in a
"unsignaled" or "signaled" state. The only current implementation of this is a
GL Fence.

**GL Fence**: A GL Sync Object that is inserted into the GL command stream. It
starts out unsignaled and becomes signaled whent GPU processes this point in the
command stream, implying that all previous commands have completed.

**Client Wait**: Block the client thread until the sync object becomes signaled,
or until a timeout occurs.

**Server Wait**: Tells the GPU to defer executing commands issued after the
fence until the fence signals. The client thread continues executing.

**Native GL Fence**: A GL Fence backed by a platform-specific cross-process
synchronization mechanism.

**GPU Fence Handle**: An IPC-transportable object (typically a file descriptor)
that can be used to duplicate a native GL fence into a different process's
context.

**GPU Fence**: A Chrome abstraction that owns a GPU fence handle representing a
native GL fence, usable for cross-process synchronization.

**CHROMIUM fence sync**: A command buffer specific GL fence that sequences
operations among command buffer GL contexts without requiring driver-level
execution of previous commands.


## Command buffer sequencing: CHROMIUM fence sync

This mechanism is intended to order operations between command buffer GL
contexts. It supports creating a cross-process-transportable SyncToken, but this
token is only valid for sharing among command buffer contexts. A
WaitSyncTokenCHROMIUM call does **not** ensure that the underlying GL
commands have been executed at the GPU driver level.

See the
[CHROMIUM_sync_point](/src/gpu/GLES2/extensions/CHROMIUM/CHROMIUM_sync_point.txt)
documentation for details.

Correctness of the CHROMIUM fence sync mechanism depends on the assumption that
commands issued from the command buffer service side happen in the order they
were issued in that thread. This is guaranteed to be true when they are all
issued onto the same service-side context. If the service thread switches
between multiple contexts, the order is only correct if the GL driver uses a
per-thread command queue. If that is not the case, and the driver has a
per-context queue even within a single thread, operations can happen out of
order. As a workaround, Chrome forces use of context virtualization to ensure
that only a single driver-level context is being used. See [issue
510232](http://crbug.com/510243#c23) and [issue
691102](http://crbug.com/691102).

## Driver-level wrappers: GLFence

At the lowest-level, [gl::GLFence](/src/ui/gl/gl_fence.h) and its subclasses
provide wrappers for GL/EGL fence handling methods such as `eglFenceSyncKHR` and
`eglWaitSyncKHR`. These fence objects can be shared within a GL share group.

Use the static `GLFence::IsGpuFenceSupported()` method to check at runtime if
the current platform has support for the GpuFence mechanism including
GpuFenceHandle transport.

The
[gl::GLFenceAndroidNativeFenceSync](/src/ui/gl/gl_fence_android_native_fence_sync.h)
subclass wraps the EGL_ANDROID_native_fence_sync extension mechanism that allows
creating a special EGLFence object from which a file descriptor can be
extracted, and then creating a duplicate fence object from that file descriptor
that is synchronized with the original fence.

## Cross-process transport: GpuFenceHandle

A [gfx::GpuFenceHandle](/src/ui/gfx/gpu_fence_handle.h) is an IPC-transportable
wrapper for the file descriptor or other underlying primitive object used to
duplicate a native GL fence into another process.

It has value semantics and can be copied multiple times, and then consumed
exactly one time. Consumers take ownership of the underlying resource. Current
consumers are:

* The `gl::GLFence::CreateFromGpuFenceHandle(handle)` static factory method
  creates a GLFence object such as GLFenceNativeAndroidFenceSync which owns
  the underlying resource.

* The ``gfx::GpuFence(gpu_fence_handle)` constructor takes ownership of the
  handle's resources without constructing a local fence.

* The IPC subsystem. Typical idiom is to call `gfx::CloneHandleForIPC(handle)`
  on a GpuFenceHandle retrieved from an owning object to create a copied handle
  that will be owned by the IPC subsystem.

## Sample Code

A usage example for two-process synchronization is to sequence access to a
globally shared drawable such as an AHardwareBuffer on Android, where the
writer uses a local GL context and the reader is a command buffer context in
the GPU process. The writer process draws into an AHardwareBuffer-backed
GLImage in the local GL context, then creates a gpu fence to mark the end of
drawing operations:

```c++
    // ... write to the shared drawable in local context, then create
    // a local fence.
    gl::GLFenceEGLNativeSync local_fence;

    // Get a GpuFence from it via handle.
    gpu::GpuFence gpu_fence(
        local_fence->GetGpuFenceHandle());
    // It's ok for local_fence to be destroyed at this point.

    // Create a matching gpu fence on the command buffer context, issue
    // server wait, and destroy it.
    EGLint id = gl->DuplicateGpuFenceCHROMIUM(&gpu_fence);
    gl->WaitGpuFenceCHROMIUM(id);
    gl->DestroyGpuFenceCHROMIUM(id);

    // ... read from the shared drawable via command buffer. These reads
    // will happen after the local_fence has signalled. The local
    // fence doesn't need to remain alive for this.
```


If a process wants to consume a drawable that was produced through a command
buffer context in the GPU process, the sequence is as follows:

```c++
    // Set up callback that's waiting for the drawable to be ready.
    void callback(const gpu::GpuFenceHandle& handle) {
        // Create a local context GL fence from the GpuFence.
        std::unique_ptr<gl::GLFence> local_fence =
            gl::GLFence::CreateFromGpuFenceHandle(handle);
        local_fence->ServerWait();
        // ... read from the shared drawable in the local context.
    }

    // ... write to the shared drawable via command buffer, then
    // create a gpu fence:
    EGLint id = gl->CreateGpuFenceCHROMIUM();
    context_support->GetGpuFenceHandle(id, callback);
    gl->DestroyGpuFenceCHROMIUM(id);
```

It is legal to create the GpuFence on a separate command buffer context instead
of on the command buffer channel that did the drawing operations, but in that
case gl->WaitSyncTokenCHROMIUM() or equivalent must be used to sequence the
operations between the distinct command buffer contexts as usual.
