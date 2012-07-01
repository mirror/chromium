// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "native_client/src/trusted/plugin/pnacl_translate_thread.h"

#include "native_client/src/trusted/desc/nacl_desc_wrapper.h"
#include "native_client/src/trusted/plugin/plugin.h"
#include "native_client/src/trusted/plugin/pnacl_resources.h"
#include "native_client/src/trusted/plugin/srpc_params.h"
#include "native_client/src/trusted/plugin/utility.h"

namespace plugin {

PnaclTranslateThread::PnaclTranslateThread() : subprocesses_should_die_(false),
                                               manifest_(NULL),
                                               ld_manifest_(NULL),
                                               obj_file_(NULL),
                                               nexe_file_(NULL),
                                               pexe_wrapper_(NULL),
                                               coordinator_error_info_(NULL),
                                               resources_(NULL),
                                               plugin_(NULL) {
  NaClXMutexCtor(&subprocess_mu_);
}

void PnaclTranslateThread::RunTranslate(
    const pp::CompletionCallback& finish_callback,
    const Manifest* manifest,
    const Manifest* ld_manifest,
    LocalTempFile* obj_file,
    LocalTempFile* nexe_file,
    nacl::DescWrapper* pexe_wrapper,
    ErrorInfo* error_info,
    PnaclResources* resources,
    Plugin* plugin) {
  PLUGIN_PRINTF(("PnaclTranslateThread::RunTranslate)\n"));
  manifest_ = manifest;
  ld_manifest_ = ld_manifest;
  obj_file_ = obj_file;
  nexe_file_ = nexe_file;
  pexe_wrapper_ = pexe_wrapper;
  coordinator_error_info_ = error_info;
  resources_ = resources;
  plugin_ = plugin;

  // Invoke llc followed by ld off the main thread.  This allows use of
  // blocking RPCs that would otherwise block the JavaScript main thread.
  report_translate_finished_ = finish_callback;
  translate_thread_.reset(new NaClThread);
  if (translate_thread_ == NULL) {
    TranslateFailed("could not allocate thread struct.");
    return;
  }
  const int32_t kArbitraryStackSize = 128 * 1024;
  if (!NaClThreadCreateJoinable(translate_thread_.get(),
                                DoTranslateThread,
                                this,
                                kArbitraryStackSize)) {
    TranslateFailed("could not create thread.");
  }
}

NaClSubprocess* PnaclTranslateThread::StartSubprocess(
    const nacl::string& url_for_nexe,
    const Manifest* manifest,
    ErrorInfo* error_info) {
  PLUGIN_PRINTF(("PnaclTranslateThread::StartSubprocess (url_for_nexe=%s)\n",
                 url_for_nexe.c_str()));
  nacl::DescWrapper* wrapper = resources_->WrapperForUrl(url_for_nexe);
  nacl::scoped_ptr<NaClSubprocess> subprocess(
      plugin_->LoadHelperNaClModule(wrapper, manifest, error_info));
  if (subprocess.get() == NULL) {
    PLUGIN_PRINTF((
        "PnaclTranslateThread::StartSubprocess: subprocess creation failed\n"));
    return NULL;
  }
  return subprocess.release();
}

void WINAPI PnaclTranslateThread::DoTranslateThread(void* arg) {
  PnaclTranslateThread* translator =
      reinterpret_cast<PnaclTranslateThread*>(arg);
  translator->DoTranslate();
}

void PnaclTranslateThread::DoTranslate() {
  ErrorInfo error_info;
  nacl::scoped_ptr<NaClSubprocess> llc_subprocess(
      StartSubprocess(PnaclUrls::GetLlcUrl(), manifest_, &error_info));
  if (llc_subprocess == NULL) {
    TranslateFailed("Compile process could not be created: " +
                    error_info.message());
    return;
  }
  // Run LLC.
  SrpcParams params;
  nacl::DescWrapper* llc_out_file = obj_file_->write_wrapper();
  PluginReverseInterface* llc_reverse =
      llc_subprocess->service_runtime()->rev_interface();
  llc_reverse->AddQuotaManagedFile(obj_file_->identifier(),
                                   obj_file_->write_file_io());
  if (!llc_subprocess->InvokeSrpcMethod("RunWithDefaultCommandLine",
                                        "hh",
                                        &params,
                                        pexe_wrapper_->desc(),
                                        llc_out_file->desc())) {
    TranslateFailed("compile failed.");
    return;
  }
  // LLC returns values that are used to determine how linking is done.
  int is_shared_library = (params.outs()[0]->u.ival != 0);
  nacl::string soname = params.outs()[1]->arrays.str;
  nacl::string lib_dependencies = params.outs()[2]->arrays.str;
  PLUGIN_PRINTF(("PnaclCoordinator: compile (translator=%p) succeeded"
                 " is_shared_library=%d, soname='%s', lib_dependencies='%s')\n",
                 this, is_shared_library, soname.c_str(),
                 lib_dependencies.c_str()));
  // Shut down the llc subprocess.
  llc_subprocess.reset(NULL);
  if (SubprocessesShouldDie()) {
    TranslateFailed("stopped by coordinator.");
    return;
  }
  if(!RunLdSubprocess(is_shared_library, soname, lib_dependencies)) {
    return;
  }
  pp::Core* core = pp::Module::Get()->core();
  core->CallOnMainThread(0, report_translate_finished_, PP_OK);
}

bool PnaclTranslateThread::RunLdSubprocess(int is_shared_library,
                                           const nacl::string& soname,
                                           const nacl::string& lib_dependencies
                                           ) {
  ErrorInfo error_info;
  nacl::scoped_ptr<NaClSubprocess> ld_subprocess(
      StartSubprocess(PnaclUrls::GetLdUrl(), ld_manifest_, &error_info));
  if (ld_subprocess == NULL) {
    TranslateFailed("Link process could not be created: " +
                    error_info.message());
    return false;
  }
  // Run LD.
  SrpcParams params;
  nacl::DescWrapper* ld_in_file = obj_file_->read_wrapper();
  nacl::DescWrapper* ld_out_file = nexe_file_->write_wrapper();
  PluginReverseInterface* ld_reverse =
      ld_subprocess->service_runtime()->rev_interface();
  ld_reverse->AddQuotaManagedFile(nexe_file_->identifier(),
                                  nexe_file_->write_file_io());
  if (!ld_subprocess->InvokeSrpcMethod("RunWithDefaultCommandLine",
                                       "hhiss",
                                       &params,
                                       ld_in_file->desc(),
                                       ld_out_file->desc(),
                                       is_shared_library,
                                       soname.c_str(),
                                       lib_dependencies.c_str())) {
    TranslateFailed("link failed.");
    return false;
  }
  PLUGIN_PRINTF(("PnaclCoordinator: link (translator=%p) succeeded\n",
                 this));
  // Shut down the ld subprocess.
  ld_subprocess.reset(NULL);
  if (SubprocessesShouldDie()) {
    TranslateFailed("stopped by coordinator.");
    return false;
  }
  return true;
}

void PnaclTranslateThread::TranslateFailed(const nacl::string& error_string) {
  PLUGIN_PRINTF(("PnaclTranslateThread::TranslateFailed (error_string='%s')\n",
                 error_string.c_str()));
  pp::Core* core = pp::Module::Get()->core();
  if (coordinator_error_info_->message().empty()) {
    // Only use our message if one hasn't already been set by the coordinator
    // (e.g. pexe load failed).
    coordinator_error_info_->SetReport(ERROR_UNKNOWN,
                                       nacl::string("PnaclCoordinator: ") +
                                       error_string);
  }
  core->CallOnMainThread(0, report_translate_finished_, PP_ERROR_FAILED);
}

bool PnaclTranslateThread::SubprocessesShouldDie() {
  nacl::MutexLocker ml(&subprocess_mu_);
  return subprocesses_should_die_;
}

void PnaclTranslateThread::SetSubprocessesShouldDie() {
  PLUGIN_PRINTF(("PnaclTranslateThread::SetSubprocessesShouldDie\n"));
  nacl::MutexLocker ml(&subprocess_mu_);
  subprocesses_should_die_ = true;
}

PnaclTranslateThread::~PnaclTranslateThread() {
  PLUGIN_PRINTF(("~PnaclTranslateThread (translate_thread=%p)\n",
                 translate_thread_.get()));
  if (translate_thread_ != NULL) {
    SetSubprocessesShouldDie();
    NaClThreadJoin(translate_thread_.get());
    PLUGIN_PRINTF(("~PnaclTranslateThread joined\n"));
  }
  NaClMutexDtor(&subprocess_mu_);
}

} // namespace plugin
