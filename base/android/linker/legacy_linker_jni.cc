// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the version of the Android-specific Chromium linker that uses
// the crazy linker to load libraries.

// This source code *cannot* depend on anything from base/ or the C++
// STL, to keep the final library small, and avoid ugly dependency issues.

#include "legacy_linker_jni.h"

#include <crazy_linker.h>
#include <fcntl.h>
#include <jni.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "linker_jni.h"

namespace chromium_android_linker {
namespace {

// Retrieve the SDK build version and pass it into the crazy linker. This
// needs to be done early in initialization, before any other crazy linker
// code is run.
// |env| is the current JNI environment handle.
// On success, return true.
bool InitSDKVersionInfo(JNIEnv* env) {
  jint value = 0;
  if (!InitStaticInt(env, "android/os/Build$VERSION", "SDK_INT", &value))
    return false;

  crazy_set_sdk_build_version(static_cast<int>(value));
  LOG_INFO("Set SDK build version to %d", static_cast<int>(value));

  return true;
}

// The linker uses a single crazy_context_t object created on demand.
// There is no need to protect this against concurrent access, locking
// is already handled on the Java side.
crazy_context_t* GetCrazyContext() {
  static crazy_context_t* s_crazy_context = nullptr;

  if (!s_crazy_context) {
    // Create new context.
    s_crazy_context = crazy_context_create();

    // Ensure libraries located in the same directory as the linker
    // can be loaded before system ones.
    crazy_context_add_search_path_for_address(
        s_crazy_context, reinterpret_cast<void*>(&GetCrazyContext));
  }

  return s_crazy_context;
}

// A scoped crazy_library_t that automatically closes the handle
// on scope exit, unless Release() has been called. The main difference
// with std::unique_ptr is the GetPtr() method that returns the address
// of the pointed-to object within the ScopedLibrary instance.
class ScopedLibrary {
 public:
  ScopedLibrary() : lib_(nullptr) {}

  ~ScopedLibrary() {
    if (lib_)
      crazy_library_close(lib_);
  }

  // Return crazy library instance.
  crazy_library_t* Get() { return lib_; }

  // Return address of crazy library instance within the ScopedLibrary
  // instance. This is useful to pass as an argument to functions that
  // want to write a new crazy_library_t* value there.
  crazy_library_t** GetPtr() { return &lib_; }

  // Release the crazy_library_t* value and return it.
  crazy_library_t* Release() {
    crazy_library_t* ret = lib_;
    lib_ = nullptr;
    return ret;
  }

 private:
  crazy_library_t* lib_;
};

// Generic library loading library. |env| should be a valid JNIEnv* value
// for the current thread, |library_name| is the library name and/or path,
// while |optional_zip_file_name| is an optional zip file path.
// |load_address| is the desired load address, and |lib_info_obj| is a
// Java reference to LibInfo Java object.
//
// If |optional_zip_file_name| is not nullptr, it must point to a zip file
// which will be searched for the library and its dependencies. Otherwise
// the loader looks up the current file system.
//
// On success, return true, and sets the fields of |lib_info_obj|
// appropriately. On failure, return false.
//
// NOTE: The corresponding crazy_library_t instance is leaked since it will
//       never be unloaded.
bool GenericLoadLibrary(JNIEnv* env,
                        const char* library_name,
                        const char* optional_zip_file_name,
                        size_t load_address,
                        jobject lib_info_obj) {
  LOG_INFO("Called for %s, at address 0x%llx", library_name,
           (unsigned long long)load_address);
  crazy_context_t* context = GetCrazyContext();

  if (!IsValidAddress(load_address)) {
    LOG_ERROR("Invalid address 0x%llx",
              static_cast<unsigned long long>(load_address));
    return false;
  }

  // Set the desired load address (0 means randomize it).
  crazy_context_set_load_address(context, load_address);

  ScopedLibrary library;
  if (optional_zip_file_name) {
    // Open from a zip file.
    if (!crazy_library_open_in_zip_file(
            library.GetPtr(), optional_zip_file_name, library_name, context)) {
      LOG_ERROR("Could not open %s in zip file %s: %s", library_name,
                optional_zip_file_name, crazy_context_get_error(context));
      return false;
    }
  } else {
    // Open directly from a file or library name.
    if (!crazy_library_open(library.GetPtr(), library_name, context)) {
      LOG_ERROR("Could not open %s: %s", library_name,
                crazy_context_get_error(context));
      return false;
    }
  }

  crazy_library_info_t info;
  if (!crazy_library_get_info(library.Get(), context, &info)) {
    LOG_ERROR("Could not get library information for %s: %s",
              library_name, crazy_context_get_error(context));
    return false;
  }

  // Release library object to keep it alive after the function returns.
  library.Release();

  s_lib_info_fields.SetLoadInfo(env,
                                lib_info_obj,
                                info.load_address, info.load_size);
  LOG_INFO("Success loading library %s", library_name);
  return true;
}

// Load a library with the chromium linker. This will also call its
// JNI_OnLoad() method, which shall register its methods. Note that
// lazy native method resolution will _not_ work after this, because
// Dalvik uses the system's dlsym() which won't see the new library,
// so explicit registration is mandatory.
//
// |env| is the current JNI environment handle.
// |clazz| is the static class handle for org.chromium.base.Linker,
// and is ignored here.
// |library_name| is the library name (e.g. libfoo.so).
// |load_address| is an explicit load address.
// |library_info| is a LibInfo handle used to communicate information
// with the Java side.
// Return true on success.
jboolean LoadLibrary(JNIEnv* env,
                     jclass clazz,
                     jstring library_name,
                     jlong load_address,
                     jobject lib_info_obj) {
  String lib_name(env, library_name);

  return GenericLoadLibrary(env, lib_name.c_str(),
                            nullptr /* optional_zip_file_name */,
                            static_cast<size_t>(load_address), lib_info_obj);
}

// Load a library from a zipfile with the chromium linker. The
// library in the zipfile must be uncompressed and page aligned.
// The basename of the library is given. The library is expected
// to be lib/<abi_tag>/crazy.<basename>. The <abi_tag> used will be the
// same as the abi for this linker. The "crazy." prefix is included
// so that the Android Package Manager doesn't extract the library into
// /data/app-lib.
//
// Loading the library will also call its JNI_OnLoad() method, which
// shall register its methods. Note that lazy native method resolution
// will _not_ work after this, because Dalvik uses the system's dlsym()
// which won't see the new library, so explicit registration is mandatory.
//
// |env| is the current JNI environment handle.
// |clazz| is the static class handle for org.chromium.base.Linker,
// and is ignored here.
// |zipfile_name| is the filename of the zipfile containing the library.
// |library_name| is the library base name (e.g. libfoo.so).
// |load_address| is an explicit load address.
// |library_info| is a LibInfo handle used to communicate information
// with the Java side.
// Returns true on success.
jboolean LoadLibraryInZipFile(JNIEnv* env,
                              jclass clazz,
                              jstring zipfile_name,
                              jstring library_name,
                              jlong load_address,
                              jobject lib_info_obj) {
  String zipfile_name_str(env, zipfile_name);
  String lib_name(env, library_name);

  return GenericLoadLibrary(env, lib_name.c_str(), zipfile_name_str.c_str(),
                            static_cast<size_t>(load_address), lib_info_obj);
}

// Class holding the Java class and method ID for the Java side
// LegacyLinker.runPendingLinkerTasksOnMainThread() method.
struct JavaCallbackBindings {
  jclass clazz;
  jmethodID method_id;

  // Initialize an instance.
  bool Init(JNIEnv* env, jclass linker_class) {
    clazz = reinterpret_cast<jclass>(env->NewGlobalRef(linker_class));
    return InitStaticMethodId(env, linker_class,
                              "runPendingLinkerTasksOnMainThread", "()V",
                              &method_id);
  }

  // Invoke the Java method through JNI. |env| must be valid for the
  // current thread.
  void Invoke(JNIEnv* env) {
    env->CallStaticVoidMethod(clazz, method_id);

    // Back out if we encounter a JNI exception.
    if (env->ExceptionCheck() == JNI_TRUE) {
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
  }
};

static JavaCallbackBindings s_java_callback_bindings;

// Called from Java to apply pending linker tasks. This shall happen
// inside the main/UI thread, after an AsyncTask was posted from
// PendingLinkerTasksNotification() below.
void nativeRunPendingLinkerTasks(JNIEnv*, jclass) {
  LOG_INFO("Called back from java to apply pending linker tasks");
  crazy_apply_pending_tasks();
}

// Called by the crazy linker during library load/unload operations to notify
// the client that there are pending tasks to be performed. This is normally
// done by calling crazy_linker_apply_pending_tasks().
//
// The goal of this function is to determine if this code is running on the
// main thread. If so, crazy_linker_apply_pending_tasks() can be called
// immediately. Otherwise, an AsyncTask should be posted to it, through
// the method referenced by |s_java_callback_bindings|.
//
// The AsyncTask will invoke nativeRunPendingLinkerTasks() above later on the
// main thread's context.
//
// |opaque| must be the JavaVM* value that was created during
// LegacyLinkerJNIInit().
static void PendingLinkerTasksNotification(void* opaque) {
  auto* vm = static_cast<JavaVM*>(opaque);

  // This can run on any thread, so create a new JNIEnv for it.
  JNIEnv* env;
  if (JNI_OK != vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4)) {
    LOG_ERROR("Could not create JNIEnv");
    return;
  }

  s_java_callback_bindings.Invoke(env);
}

jboolean CreateSharedRelro(JNIEnv* env,
                           jclass clazz,
                           jstring library_name,
                           jlong load_address,
                           jobject lib_info_obj) {
  String lib_name(env, library_name);

  LOG_INFO("Called for %s", lib_name.c_str());

  if (!IsValidAddress(load_address)) {
    LOG_ERROR("Invalid address 0x%llx",
              static_cast<unsigned long long>(load_address));
    return false;
  }

  ScopedLibrary library;
  if (!crazy_library_find_by_name(lib_name.c_str(), library.GetPtr())) {
    LOG_ERROR("Could not find %s", lib_name.c_str());
    return false;
  }

  crazy_context_t* context = GetCrazyContext();
  size_t relro_start = 0;
  size_t relro_size = 0;
  int relro_fd = -1;

  if (!crazy_library_create_shared_relro(library.Get(),
                                         context,
                                         static_cast<size_t>(load_address),
                                         &relro_start,
                                         &relro_size,
                                         &relro_fd)) {
    LOG_ERROR("Could not create shared RELRO sharing for %s: %s\n",
              lib_name.c_str(), crazy_context_get_error(context));
    return false;
  }

  s_lib_info_fields.SetRelroInfo(env,
                                 lib_info_obj,
                                 relro_start, relro_size, relro_fd);
  return true;
}

jboolean UseSharedRelro(JNIEnv* env,
                        jclass clazz,
                        jstring library_name,
                        jobject lib_info_obj) {
  String lib_name(env, library_name);

  LOG_INFO("Called for %s, lib_info_ref=%p", lib_name.c_str(), lib_info_obj);

  ScopedLibrary library;
  if (!crazy_library_find_by_name(lib_name.c_str(), library.GetPtr())) {
    LOG_ERROR("Could not find %s", lib_name.c_str());
    return false;
  }

  crazy_context_t* context = GetCrazyContext();
  size_t relro_start = 0;
  size_t relro_size = 0;
  int relro_fd = -1;
  s_lib_info_fields.GetRelroInfo(env,
                                 lib_info_obj,
                                 &relro_start, &relro_size, &relro_fd);

  LOG_INFO("library=%s relro start=%p size=%p fd=%d",
           lib_name.c_str(), (void*)relro_start, (void*)relro_size, relro_fd);

  if (!crazy_library_use_shared_relro(library.Get(),
                                      context,
                                      relro_start, relro_size, relro_fd)) {
    LOG_ERROR("Could not use shared RELRO for %s: %s",
              lib_name.c_str(), crazy_context_get_error(context));
    return false;
  }

  LOG_INFO("Library %s using shared RELRO section!", lib_name.c_str());

  return true;
}

const JNINativeMethod kNativeMethods[] = {
    {"nativeLoadLibrary",
     "("
     "Ljava/lang/String;"
     "J"
     "Lorg/chromium/base/library_loader/Linker$LibInfo;"
     ")"
     "Z",
     reinterpret_cast<void*>(&LoadLibrary)},
    {"nativeLoadLibraryInZipFile",
     "("
     "Ljava/lang/String;"
     "Ljava/lang/String;"
     "J"
     "Lorg/chromium/base/library_loader/Linker$LibInfo;"
     ")"
     "Z",
     reinterpret_cast<void*>(&LoadLibraryInZipFile)},
    {"nativeRunPendingLinkerTasks",
     "("
     ")"
     "V",
     reinterpret_cast<void*>(&nativeRunPendingLinkerTasks)},
    {"nativeCreateSharedRelro",
     "("
     "Ljava/lang/String;"
     "J"
     "Lorg/chromium/base/library_loader/Linker$LibInfo;"
     ")"
     "Z",
     reinterpret_cast<void*>(&CreateSharedRelro)},
    {"nativeUseSharedRelro",
     "("
     "Ljava/lang/String;"
     "Lorg/chromium/base/library_loader/Linker$LibInfo;"
     ")"
     "Z",
     reinterpret_cast<void*>(&UseSharedRelro)},
};

const size_t kNumNativeMethods =
    sizeof(kNativeMethods) / sizeof(kNativeMethods[0]);

}  // namespace

bool LegacyLinkerJNIInit(JavaVM* vm, JNIEnv* env) {
  LOG_INFO("Entering");

  // Initialize SDK version info.
  LOG_INFO("Retrieving SDK version info");
  if (!InitSDKVersionInfo(env))
    return false;

  // Register native methods.
  jclass linker_class;
  if (!InitClassReference(env,
                          "org/chromium/base/library_loader/LegacyLinker",
                          &linker_class))
    return false;

  LOG_INFO("Registering native methods");
  if (env->RegisterNatives(linker_class, kNativeMethods, kNumNativeMethods) < 0)
    return false;

  // Resolve and save the Java side Linker callback class and method.
  LOG_INFO("Resolving callback bindings");
  if (!s_java_callback_bindings.Init(env, linker_class)) {
    return false;
  }

  // Save JavaVM* handle into context.
  crazy_context_t* context = GetCrazyContext();
  crazy_context_set_java_vm(context, vm, JNI_VERSION_1_4);

  // Register the function that the crazy linker can call to post code
  // for later execution.
  crazy_set_pending_task_notification(&PendingLinkerTasksNotification, vm);

  return true;
}

}  // namespace chromium_android_linker
