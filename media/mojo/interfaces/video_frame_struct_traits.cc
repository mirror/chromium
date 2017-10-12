// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/video_frame_struct_traits.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task_scheduler/post_task.h"
#include "media/mojo/common/mojo_shared_buffer_video_frame.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace mojo {

namespace {

// Implementation of media.mojom.VideoFrameReleaser, which holds a VideoFrame
// ref as long as the connection is not closed.
class VideoFrameReleaser : public media::mojom::VideoFrameReleaser,
                           public media::VideoFrame::SyncTokenClient {
 public:
  VideoFrameReleaser(scoped_refptr<media::VideoFrame> video_frame)
      : video_frame_(std::move(video_frame)) {}

  void UpdateReleaseSyncToken(const gpu::SyncToken& sync_token) final {
    // TODO(sandersd): Add a more direct setter so that we don't need to be a
    // SyncTokenClient.
    sync_token_ = sync_token;
    video_frame_->UpdateReleaseSyncToken(this);
  }

  void GenerateSyncToken(gpu::SyncToken* sync_token) final {
    *sync_token = sync_token_;
  }

  void WaitSyncToken(const gpu::SyncToken& sync_token) final {
    // NOP, we don't care what the old sync token was.
  }

 private:
  scoped_refptr<media::VideoFrame> video_frame_;
  gpu::SyncToken sync_token_;
};

void NotifyRelease(media::mojom::VideoFrameReleaserPtrInfo releaser_info,
                   const gpu::SyncToken& sync_token) {
  DCHECK(releaser_info.is_valid());
  media::mojom::VideoFrameReleaserPtr releaser;
  releaser.Bind(std::move(releaser_info));
  releaser->UpdateReleaseSyncToken(sync_token);
}

void BindReleaserOnSequence(scoped_refptr<media::VideoFrame> video_frame,
                            media::mojom::VideoFrameReleaserRequest request) {
  StrongBinding<media::mojom::VideoFrameReleaser>::Create(
      base::MakeUnique<VideoFrameReleaser>(std::move(video_frame)),
      std::move(request));
}

}  // namespace

// static
media::mojom::VideoFrameReleaserPtr
StructTraits<media::mojom::VideoFrameDataView,
             scoped_refptr<media::VideoFrame>>::
    releaser(const scoped_refptr<media::VideoFrame>& input) {
  // TODO(sandersd): If there is no release callback or destruction observer,
  // return nullptr.
  media::mojom::VideoFrameReleaserPtr video_frame_releaser;
  // Note: Binds |video_frame_releaser| on the current task runner. This is
  // probably okay since we are about to send it across the channel anyway.
  media::mojom::VideoFrameReleaserRequest request =
      MakeRequest(&video_frame_releaser);
  // Schedule construction of the channel listener on its own sequence.
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  task_runner->PostTask(FROM_HERE, base::Bind(&BindReleaserOnSequence, input,
                                              base::Passed(&request)));
  return video_frame_releaser;
}

// static
media::mojom::VideoFrameDataPtr StructTraits<media::mojom::VideoFrameDataView,
                                             scoped_refptr<media::VideoFrame>>::
    data(const scoped_refptr<media::VideoFrame>& input) {
  if (input->metadata()->IsTrue(media::VideoFrameMetadata::END_OF_STREAM)) {
    return media::mojom::VideoFrameData::NewEosData(
        media::mojom::EosVideoFrameData::New());
  }

  if (input->storage_type() == media::VideoFrame::STORAGE_MOJO_SHARED_BUFFER) {
    media::MojoSharedBufferVideoFrame* mojo_frame =
        static_cast<media::MojoSharedBufferVideoFrame*>(input.get());

    mojo::ScopedSharedBufferHandle dup = mojo_frame->Handle().Clone();
    DCHECK(dup.is_valid());

    return media::mojom::VideoFrameData::NewSharedBufferData(
        media::mojom::SharedBufferVideoFrameData::New(
            std::move(dup), mojo_frame->MappedSize(),
            mojo_frame->stride(media::VideoFrame::kYPlane),
            mojo_frame->stride(media::VideoFrame::kUPlane),
            mojo_frame->stride(media::VideoFrame::kVPlane),
            mojo_frame->PlaneOffset(media::VideoFrame::kYPlane),
            mojo_frame->PlaneOffset(media::VideoFrame::kUPlane),
            mojo_frame->PlaneOffset(media::VideoFrame::kVPlane)));
  }

  if (input->HasTextures()) {
    std::vector<gpu::MailboxHolder> mailbox_holder(
        media::VideoFrame::kMaxPlanes);
    size_t num_planes = media::VideoFrame::NumPlanes(input->format());
    for (size_t i = 0; i < num_planes; i++)
      mailbox_holder[i] = input->mailbox_holder(i);
    return media::mojom::VideoFrameData::NewMailboxData(
        media::mojom::MailboxVideoFrameData::New(std::move(mailbox_holder)));
  }

  NOTREACHED() << "Unsupported VideoFrame conversion";
  return nullptr;
}

// static
bool StructTraits<media::mojom::VideoFrameDataView,
                  scoped_refptr<media::VideoFrame>>::
    Read(media::mojom::VideoFrameDataView input,
         scoped_refptr<media::VideoFrame>* output) {
  // View of the |data| member of the input media::mojom::VideoFrame.
  media::mojom::VideoFrameDataDataView data;
  input.GetDataDataView(&data);

  if (data.is_eos_data()) {
    // TODO(sandersd): Are destruction callbacks needed for EOS frames?
    *output = media::VideoFrame::CreateEOSFrame();
    return !!*output;
  }

  media::VideoPixelFormat format;
  if (!input.ReadFormat(&format))
    return false;

  gfx::Size coded_size;
  if (!input.ReadCodedSize(&coded_size))
    return false;

  gfx::Rect visible_rect;
  if (!input.ReadVisibleRect(&visible_rect))
    return false;

  if (!gfx::Rect(coded_size).Contains(visible_rect))
    return false;

  gfx::Size natural_size;
  if (!input.ReadNaturalSize(&natural_size))
    return false;

  base::TimeDelta timestamp;
  if (!input.ReadTimestamp(&timestamp))
    return false;

  scoped_refptr<media::VideoFrame> frame;
  if (data.is_shared_buffer_data()) {
    media::mojom::SharedBufferVideoFrameDataDataView shared_buffer_data;
    data.GetSharedBufferDataDataView(&shared_buffer_data);

    // TODO(sandersd): Destruction callback.

    // TODO(sandersd): Conversion from uint64_t to size_t could cause
    // corruption. Platform-dependent types should be removed from the
    // implementation (limiting to 32-bit offsets is fine).
    frame = media::MojoSharedBufferVideoFrame::Create(
        format, coded_size, visible_rect, natural_size,
        shared_buffer_data.TakeFrameData(),
        shared_buffer_data.frame_data_size(), shared_buffer_data.y_offset(),
        shared_buffer_data.u_offset(), shared_buffer_data.v_offset(),
        shared_buffer_data.y_stride(), shared_buffer_data.u_stride(),
        shared_buffer_data.v_stride(), timestamp);
  } else if (data.is_mailbox_data()) {
    media::mojom::MailboxVideoFrameDataDataView mailbox_data;
    data.GetMailboxDataDataView(&mailbox_data);

    std::vector<gpu::MailboxHolder> mailbox_holder;
    if (!mailbox_data.ReadMailboxHolder(&mailbox_holder))
      return false;

    gpu::MailboxHolder mailbox_holder_array[media::VideoFrame::kMaxPlanes];
    for (size_t i = 0; i < media::VideoFrame::kMaxPlanes; i++)
      mailbox_holder_array[i] = mailbox_holder[i];

    media::VideoFrame::ReleaseMailboxCB release_cb;
    media::mojom::VideoFrameReleaserPtr releaser =
        input.TakeReleaser<media::mojom::VideoFrameReleaserPtr>();
    if (releaser) {
      media::mojom::VideoFrameReleaserPtrInfo releaser_info =
          releaser.PassInterface();
      release_cb = base::Bind(&NotifyRelease, base::Passed(&releaser_info));
    }

    frame = media::VideoFrame::WrapNativeTextures(
        format, mailbox_holder_array, std::move(release_cb), coded_size,
        visible_rect, natural_size, timestamp);
  } else {
    // TODO(sandersd): Switch on the union tag to avoid this ugliness?
    NOTREACHED();
    return false;
  }

  std::unique_ptr<base::DictionaryValue> metadata;
  if (!input.ReadMetadata(&metadata))
    return false;
  frame->metadata()->MergeInternalValuesFrom(*metadata);

  *output = std::move(frame);
  return true;
}

}  // namespace mojo
