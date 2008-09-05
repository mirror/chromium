// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <stdlib.h>

#include "v8.h"

#include "ast.h"

using namespace v8::internal;

static void TestList() {
  List<Node*>* list = new List<Node*>(0);
  CHECK_EQ(0, list->length());

  Node* node = new EmptyStatement();
  list->Add(node);
  CHECK_EQ(1, list->length());
  CHECK_EQ(node, list->at(0));
  CHECK_EQ(node, list->last());

  const int kElements = 100;
  for (int i = 0; i < kElements; i++) {
    list->Add(node);
  }
  CHECK_EQ(1 + kElements, list->length());

  list->Clear();
  CHECK_EQ(0, list->length());
}


static void TestRemoveLast() {
  List<int> list(4);
  CHECK_EQ(0, list.length());
  list.Add(1);
  CHECK_EQ(1, list.length());
  CHECK_EQ(1, list.last());
  list.RemoveLast();
  CHECK_EQ(0, list.length());
  list.Add(2);
  list.Add(3);
  CHECK_EQ(2, list.length());
  CHECK_EQ(3, list.last());
  list.RemoveLast();
  CHECK_EQ(1, list.length());
  CHECK_EQ(2, list.last());
  list.RemoveLast();
  CHECK_EQ(0, list.length());

  const int kElements = 100;
  for (int i = 0; i < kElements; i++) list.Add(i);
  for (int j = kElements - 1; j >= 0; j--) {
    CHECK_EQ(j + 1, list.length());
    CHECK_EQ(j, list.last());
    list.RemoveLast();
    CHECK_EQ(j, list.length());
  }
}


static void TestDeleteEmpty() {
  {
    List<int>* list = new List<int>(0);
    delete list;
  }
  {
    List<int> list(0);
  }
}
