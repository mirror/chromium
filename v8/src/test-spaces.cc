// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <stdlib.h>

#include "v8.h"

using namespace v8::internal;

static void VerifyRSet(Address page_start) {
#ifdef DEBUG
  Page::set_rset_state(Page::IN_USE);
#endif

  Page* p = Page::FromAddress(page_start);

  p->ClearRSet();

  for (Address addr = p->ObjectAreaStart();
       addr < p->ObjectAreaEnd();
       addr += kPointerSize) {
    CHECK(!Page::IsRSetSet(addr, 0));
  }

  for (Address addr = p->ObjectAreaStart();
       addr < p->ObjectAreaEnd();
       addr += kPointerSize) {
    Page::SetRSet(addr, 0);
  }

  for (Address addr = p->ObjectAreaStart();
       addr < p->ObjectAreaEnd();
       addr += kPointerSize) {
    CHECK(Page::IsRSetSet(addr, 0));
  }
}


static void TestPage() {
#ifdef DEBUG
  Page::set_rset_state(Page::NOT_IN_USE);
#endif

  byte* mem = NewArray<byte>(2*Page::kPageSize);
  CHECK(mem != NULL);

  Address start = reinterpret_cast<Address>(mem);
  Address page_start = RoundUp(start, Page::kPageSize);

  Page* p = Page::FromAddress(page_start);
  CHECK(p->address() == page_start);
  CHECK(p->is_valid());

  p->opaque_header = 0;
  p->is_normal_page = 0x1;
  CHECK(!p->next_page()->is_valid());

  CHECK(p->ObjectAreaStart() == page_start + Page::kObjectStartOffset);
  CHECK(p->ObjectAreaEnd() == page_start + Page::kPageSize);

  CHECK(p->Offset(page_start + Page::kObjectStartOffset) ==
        Page::kObjectStartOffset);
  CHECK(p->Offset(page_start + Page::kPageSize) == Page::kPageSize);

  CHECK(p->OffsetToAddress(Page::kObjectStartOffset) == p->ObjectAreaStart());
  CHECK(p->OffsetToAddress(Page::kPageSize) == p->ObjectAreaEnd());

  // test remember set
  VerifyRSet(page_start);

  DeleteArray(mem);
}


static void TestMemoryAllocator() {
  CHECK(MemoryAllocator::Setup(Heap::MaxCapacity()));

  PagedSpace faked_space(Heap::MaxCapacity(), OLD_SPACE);
  int total_pages = 0;
  int requested = 2;
  int allocated;
  // If we request two pages, we should get one or two.
  Page* first_page =
      MemoryAllocator::AllocatePages(requested, &allocated, &faked_space);
  CHECK(first_page->is_valid());
  CHECK(allocated > 0 && allocated <= 2);
  total_pages += allocated;

  Page* last_page = first_page;
  for (Page* p = first_page; p->is_valid(); p = p->next_page()) {
    CHECK(MemoryAllocator::IsPageInSpace(p, &faked_space));
    last_page = p;
  }

  // Again, we should get one or two pages.
  Page* others =
      MemoryAllocator::AllocatePages(requested, &allocated, &faked_space);
  CHECK(others->is_valid());
  CHECK(allocated > 0 && allocated <= 2);
  total_pages += allocated;

  MemoryAllocator::SetNextPage(last_page, others);
  int page_count = 0;
  for (Page* p = first_page; p->is_valid(); p = p->next_page()) {
    CHECK(MemoryAllocator::IsPageInSpace(p, &faked_space));
    page_count++;
  }
  CHECK(total_pages == page_count);

  Page* second_page = first_page->next_page();
  CHECK(second_page->is_valid());

  // Freeing pages at the first chunk starting at or after the second page
  // should free the entire second chunk.  It will return the last page in the
  // first chunk (if the second page was in the first chunk) or else an
  // invalid page (if the second page was the start of the second chunk).
  Page* free_return = MemoryAllocator::FreePages(second_page);
  CHECK(free_return == last_page || !free_return->is_valid());
  MemoryAllocator::SetNextPage(first_page, free_return);

  // Freeing pages in the first chunk starting at the first page should free
  // the first chunk and return an invalid page.
  Page* invalid_page = MemoryAllocator::FreePages(first_page);
  CHECK(!invalid_page->is_valid());

  MemoryAllocator::TearDown();
}


static void TestNewSpace() {
  CHECK(Heap::ConfigureHeap(0, 0));
  CHECK(MemoryAllocator::Setup(Heap::MaxCapacity()));

  NewSpace* s = new NewSpace(Heap::InitialSemiSpaceSize(),
                             Heap::SemiSpaceSize());
  CHECK(s != NULL);

  void* chunk =
      MemoryAllocator::ReserveInitialChunk(2 * Heap::YoungGenerationSize());
  CHECK(chunk != NULL);
  Address start = RoundUp(static_cast<Address>(chunk),
                          Heap::YoungGenerationSize());
  CHECK(s->Setup(start, Heap::YoungGenerationSize()));
  CHECK(s->HasBeenSetup());

  while (s->Available() >= Page::kMaxHeapObjectSize) {
    Object* obj = s->AllocateRaw(Page::kMaxHeapObjectSize);
    CHECK(!obj->IsFailure());
    CHECK(s->Contains(HeapObject::cast(obj)));
  }

  s->TearDown();
  delete s;
  MemoryAllocator::TearDown();
}


static void TestOldSpace() {
  CHECK(Heap::ConfigureHeap(0, 0));
  CHECK(MemoryAllocator::Setup(Heap::MaxCapacity()));

  OldSpace* s = new OldSpace(Heap::OldGenerationSize(), OLD_SPACE);
  CHECK(s != NULL);

  void* chunk =
      MemoryAllocator::ReserveInitialChunk(2 * Heap::YoungGenerationSize());
  CHECK(chunk != NULL);
  Address start = static_cast<Address>(chunk);
  size_t size = RoundUp(start, Heap::YoungGenerationSize()) - start;

  CHECK(s->Setup(start, size));

  while (s->Available() > 0) {
    Object* obj = s->AllocateRaw(Page::kMaxHeapObjectSize);
    CHECK(!obj->IsFailure());
  }

  s->TearDown();
  delete s;
  MemoryAllocator::TearDown();
}


static void TestLargeObjectSpace() {
  MemoryAllocator::Setup(Heap::MaxCapacity());

  LargeObjectSpace* lo = new LargeObjectSpace();
  CHECK(lo != NULL);

  CHECK(lo->Setup());

  Map* faked_map = reinterpret_cast<Map*>(HeapObject::FromAddress(0));
  int lo_size = Page::kPageSize;

  Object* obj = lo->AllocateRaw(lo_size);
  CHECK(!obj->IsFailure());
  CHECK(obj->IsHeapObject());

  HeapObject* ho = HeapObject::cast(obj);
  ho->set_map(faked_map);

  CHECK(lo->Contains(HeapObject::cast(obj)));

  CHECK(lo->FindObject(ho->address()) == obj);

  CHECK(lo->Contains(ho));

  while (true) {
    int available = lo->Available();
    obj = lo->AllocateRaw(lo_size);
    if (obj->IsFailure()) break;
    HeapObject::cast(obj)->set_map(faked_map);
    CHECK(lo->Available() < available);
  };

  CHECK(!lo->IsEmpty());

  obj = lo->AllocateRaw(lo_size);
  CHECK(obj->IsFailure());

  lo->TearDown();
  delete lo;

  MemoryAllocator::TearDown();
}
