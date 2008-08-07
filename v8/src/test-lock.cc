// Copyright 2006-2008 Google, Inc. All Rights Reserved.
//
// Tests of the TokenLock class from lock.h

#include <stdlib.h>

#include "v8.h"

#include "platform.h"


using namespace ::v8::internal;


// Simple test of locking logic
static void TestSimple() {
  Mutex* mutex = OS::CreateMutex();
  CHECK_EQ(0, mutex->Lock());  // acquire the lock with the right token
  CHECK_EQ(0, mutex->Unlock());  // can unlock with the right token
  delete mutex;
}


static void TestMultiLock() {
  Mutex* mutex = OS::CreateMutex();
  CHECK_EQ(0, mutex->Lock());
  CHECK_EQ(0, mutex->Unlock());
  delete mutex;
}


static void TestShallowLock() {
  Mutex* mutex = OS::CreateMutex();
  CHECK_EQ(0, mutex->Lock());
  CHECK_EQ(0, mutex->Unlock());
  CHECK_EQ(0, mutex->Lock());
  CHECK_EQ(0, mutex->Unlock());
  delete mutex;
}
