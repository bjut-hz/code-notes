// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "leveldb/iterator.h"

namespace leveldb {

Iterator::Iterator() {
  cleanup_head_.function = nullptr;
  cleanup_head_.next = nullptr;
}

Iterator::~Iterator() {
  if (!cleanup_head_.IsEmpty()) {
    cleanup_head_.Run();
    for (CleanupNode* node = cleanup_head_.next; node != nullptr; ) {
      node->Run();
      CleanupNode* next_node = node->next;
      delete node;
      node = next_node;
    }
  }
}

void Iterator::RegisterCleanup(CleanupFunction func, void* arg1, void* arg2) {
  assert(func != nullptr);
  CleanupNode* node;
  if (cleanup_head_.IsEmpty()) { // 使用nullptr来判断是否是已被使用的节点。利用头结点，没有dummy节点
    node = &cleanup_head_;
  } else { // 创建新的节点，置于头结点之后。函数调用顺序与插入顺序不同，第一个函数会被首先调用，剩余的函数逆序调用
    node = new CleanupNode(); 
    node->next = cleanup_head_.next;
    cleanup_head_.next = node;
  }
  node->function = func;
  node->arg1 = arg1;
  node->arg2 = arg2;
}

// 匿名命名空间，降低类型的可见性，可见性限制在这个文件内
namespace {

class EmptyIterator : public Iterator {
 public:
  EmptyIterator(const Status& s) : status_(s) { }
  ~EmptyIterator() override = default;

  bool Valid() const override { return false; }
  void Seek(const Slice& target) override { }
  void SeekToFirst() override { }
  void SeekToLast() override { }
  void Next() override { assert(false); }
  void Prev() override { assert(false); }
  Slice key() const override { assert(false); return Slice(); }
  Slice value() const override { assert(false); return Slice(); }
  Status status() const override { return status_; }

 private:
  Status status_;
};

}  // anonymous namespace

Iterator* NewEmptyIterator() {
  return new EmptyIterator(Status::OK());
}

Iterator* NewErrorIterator(const Status& status) {
  return new EmptyIterator(status);
}

}  // namespace leveldb
