// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_renderer_host.h"

#include "base/bind.h"
#include "base/metrics/histogram.h"
#include "base/process.h"
#include "base/shared_memory.h"
#include "content/browser/renderer_host/media/audio_common.h"
#include "content/browser/renderer_host/media/audio_sync_reader.h"
#include "content/browser/renderer_host/media/media_observer.h"
#include "content/common/media/audio_messages.h"
#include "content/public/browser/resource_context.h"
#include "media/audio/audio_util.h"

using content::BrowserMessageFilter;
using content::BrowserThread;

AudioRendererHost::AudioEntry::AudioEntry()
    : stream_id(0),
      pending_close(false) {
}

AudioRendererHost::AudioEntry::~AudioEntry() {}

///////////////////////////////////////////////////////////////////////////////
// AudioRendererHost implementations.
AudioRendererHost::AudioRendererHost(
    content::ResourceContext* resource_context)
    : resource_context_(resource_context),
      media_observer_(NULL) {
}

AudioRendererHost::~AudioRendererHost() {
  DCHECK(audio_entries_.empty());
}

void AudioRendererHost::OnChannelClosing() {
  BrowserMessageFilter::OnChannelClosing();

  // Since the IPC channel is gone, close all requested audio streams.
  DeleteEntries();
}

void AudioRendererHost::OnDestruct() const {
  BrowserThread::DeleteOnIOThread::Destruct(this);
}

///////////////////////////////////////////////////////////////////////////////
// media::AudioOutputController::EventHandler implementations.
void AudioRendererHost::OnCreated(media::AudioOutputController* controller) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(
          &AudioRendererHost::DoCompleteCreation,
          this,
          make_scoped_refptr(controller)));
}

void AudioRendererHost::OnPlaying(media::AudioOutputController* controller) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(
          &AudioRendererHost::DoSendPlayingMessage,
          this,
          make_scoped_refptr(controller)));
}

void AudioRendererHost::OnPaused(media::AudioOutputController* controller) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(
          &AudioRendererHost::DoSendPausedMessage,
          this,
          make_scoped_refptr(controller)));
}

void AudioRendererHost::OnError(media::AudioOutputController* controller,
                                int error_code) {
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&AudioRendererHost::DoHandleError,
                 this, make_scoped_refptr(controller), error_code));
}

void AudioRendererHost::DoCompleteCreation(
    media::AudioOutputController* controller) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupByController(controller);
  if (!entry)
    return;

  if (!peer_handle()) {
    NOTREACHED() << "Renderer process handle is invalid.";
    DeleteEntryOnError(entry);
    return;
  }

  // Once the audio stream is created then complete the creation process by
  // mapping shared memory and sharing with the renderer process.
  base::SharedMemoryHandle foreign_memory_handle;
  if (!entry->shared_memory.ShareToProcess(peer_handle(),
                                           &foreign_memory_handle)) {
    // If we failed to map and share the shared memory then close the audio
    // stream and send an error message.
    DeleteEntryOnError(entry);
    return;
  }

  AudioSyncReader* reader =
      static_cast<AudioSyncReader*>(entry->reader.get());

#if defined(OS_WIN)
  base::SyncSocket::Handle foreign_socket_handle;
#else
  base::FileDescriptor foreign_socket_handle;
#endif

  // If we failed to prepare the sync socket for the renderer then we fail
  // the construction of audio stream.
  if (!reader->PrepareForeignSocketHandle(peer_handle(),
                                          &foreign_socket_handle)) {
    DeleteEntryOnError(entry);
    return;
  }

  Send(new AudioMsg_NotifyStreamCreated(
      entry->stream_id,
      foreign_memory_handle,
      foreign_socket_handle,
      media::PacketSizeSizeInBytes(entry->shared_memory.created_size())));
}

void AudioRendererHost::DoSendPlayingMessage(
    media::AudioOutputController* controller) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupByController(controller);
  if (!entry)
    return;

  Send(new AudioMsg_NotifyStreamStateChanged(
    entry->stream_id, kAudioStreamPlaying));
}

void AudioRendererHost::DoSendPausedMessage(
    media::AudioOutputController* controller) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupByController(controller);
  if (!entry)
    return;

  Send(new AudioMsg_NotifyStreamStateChanged(
      entry->stream_id, kAudioStreamPaused));
}

void AudioRendererHost::DoHandleError(media::AudioOutputController* controller,
                                      int error_code) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupByController(controller);
  if (!entry)
    return;

  DeleteEntryOnError(entry);
}

///////////////////////////////////////////////////////////////////////////////
// IPC Messages handler
bool AudioRendererHost::OnMessageReceived(const IPC::Message& message,
                                          bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(AudioRendererHost, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(AudioHostMsg_CreateStream, OnCreateStream)
    IPC_MESSAGE_HANDLER(AudioHostMsg_PlayStream, OnPlayStream)
    IPC_MESSAGE_HANDLER(AudioHostMsg_PauseStream, OnPauseStream)
    IPC_MESSAGE_HANDLER(AudioHostMsg_FlushStream, OnFlushStream)
    IPC_MESSAGE_HANDLER(AudioHostMsg_CloseStream, OnCloseStream)
    IPC_MESSAGE_HANDLER(AudioHostMsg_SetVolume, OnSetVolume)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()

  return handled;
}

void AudioRendererHost::OnCreateStream(
    int stream_id, const AudioParameters& params) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(LookupById(stream_id) == NULL);

  AudioParameters audio_params(params);

  // Select the hardware packet size if not specified.
  if (!audio_params.samples_per_packet) {
    audio_params.samples_per_packet = SelectSamplesPerPacket(audio_params);
  }
  uint32 packet_size = audio_params.GetPacketSize();

  scoped_ptr<AudioEntry> entry(new AudioEntry());

  // Create the shared memory and share with the renderer process.
  uint32 shared_memory_size =
      media::TotalSharedMemorySizeInBytes(packet_size);
  if (!entry->shared_memory.CreateAndMapAnonymous(shared_memory_size)) {
    // If creation of shared memory failed then send an error message.
    SendErrorMessage(stream_id);
    return;
  }

  // Create sync reader and try to initialize it.
  scoped_ptr<AudioSyncReader> reader(
      new AudioSyncReader(&entry->shared_memory));

  if (!reader->Init()) {
    SendErrorMessage(stream_id);
    return;
  }

  // If we have successfully created the SyncReader then assign it to the
  // entry and construct an AudioOutputController.
  entry->reader.reset(reader.release());
  entry->controller = media::AudioOutputController::Create(
      resource_context_->GetAudioManager(), this, audio_params,
      entry->reader.get());

  if (!entry->controller) {
    SendErrorMessage(stream_id);
    return;
  }

  // If we have created the controller successfully, create an entry and add it
  // to the map.
  entry->stream_id = stream_id;
  audio_entries_.insert(std::make_pair(stream_id, entry.release()));
  media_observer()->OnSetAudioStreamStatus(this, stream_id, "created");
}

void AudioRendererHost::OnPlayStream(int stream_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupById(stream_id);
  if (!entry) {
    SendErrorMessage(stream_id);
    return;
  }

  entry->controller->Play();
  media_observer()->OnSetAudioStreamPlaying(this, stream_id, true);
}

void AudioRendererHost::OnPauseStream(int stream_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupById(stream_id);
  if (!entry) {
    SendErrorMessage(stream_id);
    return;
  }

  entry->controller->Pause();
  media_observer()->OnSetAudioStreamPlaying(this, stream_id, false);
}

void AudioRendererHost::OnFlushStream(int stream_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupById(stream_id);
  if (!entry) {
    SendErrorMessage(stream_id);
    return;
  }

  entry->controller->Flush();
  media_observer()->OnSetAudioStreamStatus(this, stream_id, "flushed");
}

void AudioRendererHost::OnCloseStream(int stream_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  media_observer()->OnSetAudioStreamStatus(this, stream_id, "closed");

  AudioEntry* entry = LookupById(stream_id);

  if (entry)
    CloseAndDeleteStream(entry);
}

void AudioRendererHost::OnSetVolume(int stream_id, double volume) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntry* entry = LookupById(stream_id);
  if (!entry) {
    SendErrorMessage(stream_id);
    return;
  }

  // Make sure the volume is valid.
  if (volume < 0 || volume > 1.0)
    return;
  entry->controller->SetVolume(volume);
  media_observer()->OnSetAudioStreamVolume(this, stream_id, volume);
}

void AudioRendererHost::SendErrorMessage(int32 stream_id) {
  Send(new AudioMsg_NotifyStreamStateChanged(stream_id, kAudioStreamError));
}

void AudioRendererHost::DeleteEntries() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  for (AudioEntryMap::iterator i = audio_entries_.begin();
       i != audio_entries_.end(); ++i) {
    CloseAndDeleteStream(i->second);
  }
}

void AudioRendererHost::CloseAndDeleteStream(AudioEntry* entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!entry->pending_close) {
    entry->controller->Close(
        base::Bind(&AudioRendererHost::OnStreamClosed, this, entry));
    entry->pending_close = true;
  }
}

void AudioRendererHost::OnStreamClosed(AudioEntry* entry) {
  // Delete the entry on the IO thread after we've closed the stream.
  // (We're currently on the audio thread).
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&AudioRendererHost::DeleteEntry, this, entry));
}

void AudioRendererHost::DeleteEntry(AudioEntry* entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Delete the entry when this method goes out of scope.
  scoped_ptr<AudioEntry> entry_deleter(entry);

  // Erase the entry identified by |stream_id| from the map.
  audio_entries_.erase(entry->stream_id);

  // Notify the media observer.
  media_observer()->OnDeleteAudioStream(this, entry->stream_id);
}

void AudioRendererHost::DeleteEntryOnError(AudioEntry* entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Sends the error message first before we close the stream because
  // |entry| is destroyed in DeleteEntry().
  SendErrorMessage(entry->stream_id);

  media_observer()->OnSetAudioStreamStatus(this, entry->stream_id, "error");
  CloseAndDeleteStream(entry);
}

AudioRendererHost::AudioEntry* AudioRendererHost::LookupById(int stream_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  AudioEntryMap::iterator i = audio_entries_.find(stream_id);
  if (i != audio_entries_.end() && !i->second->pending_close)
    return i->second;
  return NULL;
}

AudioRendererHost::AudioEntry* AudioRendererHost::LookupByController(
    media::AudioOutputController* controller) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Iterate the map of entries.
  // TODO(hclam): Implement a faster look up method.
  for (AudioEntryMap::iterator i = audio_entries_.begin();
       i != audio_entries_.end(); ++i) {
    if (!i->second->pending_close && controller == i->second->controller.get())
      return i->second;
  }
  return NULL;
}

MediaObserver* AudioRendererHost::media_observer() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!media_observer_)
    media_observer_ = resource_context_->GetMediaObserver();
  return media_observer_;
}
