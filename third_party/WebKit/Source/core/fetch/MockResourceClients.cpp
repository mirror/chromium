// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fetch/MockResourceClients.h"

#include "core/fetch/ImageResource.h"
#include "core/fetch/ImageResourceContent.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

MockResourceClient::MockResourceClient(Resource* resource)
    : m_resource(resource),
      m_notifyFinishedCalled(false),
      m_encodedSizeOnNotifyFinished(0) {
  ThreadState::current()->registerPreFinalizer(this);
  m_resource->addClient(this);
}

MockResourceClient::~MockResourceClient() {}

void MockResourceClient::notifyFinished(Resource* resource) {
  ASSERT_FALSE(m_notifyFinishedCalled);
  m_notifyFinishedCalled = true;
  m_encodedSizeOnNotifyFinished = resource->encodedSize();
}

void MockResourceClient::removeAsClient() {
  m_resource->removeClient(this);
  m_resource = nullptr;
}

void MockResourceClient::dispose() {
  if (m_resource) {
    m_resource->removeClient(this);
    m_resource = nullptr;
  }
}

DEFINE_TRACE(MockResourceClient) {
  visitor->trace(m_resource);
  ResourceClient::trace(visitor);
}

MockImageResourceClient::MockImageResourceClient(ImageResource* resource)
    : MockResourceClient(resource),
      m_imageChangedCount(0),
      m_encodedSizeOnLastImageChanged(0),
      m_imageNotifyFinishedCount(0),
      m_encodedSizeOnImageNotifyFinished(0) {
  toImageResource(m_resource)->getContent()->addObserver(this);
}

MockImageResourceClient::~MockImageResourceClient() {}

void MockImageResourceClient::removeAsClient() {
  toImageResource(m_resource)->getContent()->removeObserver(this);
  MockResourceClient::removeAsClient();
}

void MockImageResourceClient::dispose() {
  if (m_resource)
    toImageResource(m_resource)->getContent()->removeObserver(this);
  MockResourceClient::dispose();
}

void MockImageResourceClient::imageChanged(ImageResourceContent* image,
                                           const IntRect*) {
  m_imageChangedCount++;
  m_encodedSizeOnLastImageChanged = m_resource->encodedSize();
}

void MockImageResourceClient::imageNotifyFinished(ImageResourceContent* image) {
  ASSERT_EQ(0, m_imageNotifyFinishedCount);
  m_imageNotifyFinishedCount++;
  m_encodedSizeOnImageNotifyFinished = m_resource->encodedSize();
}

bool MockImageResourceClient::notifyFinishedCalled() const {
  return m_notifyFinishedCalled;
}

}  // namespace blink
