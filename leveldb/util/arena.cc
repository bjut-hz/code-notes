// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/arena.h"

namespace leveldb {
// ÿ��block�Ĵ�СΪ4kb
static const int kBlockSize = 4096;

Arena::Arena() : memory_usage_(0) {
  alloc_ptr_ = nullptr;  // First allocation will allocate a block
  alloc_bytes_remaining_ = 0;
}

Arena::~Arena() {
  for (size_t i = 0; i < blocks_.size(); i++) {
    delete[] blocks_[i]; // �ڴ��ͷ�
  }
}

char* Arena::AllocateFallback(size_t bytes) {
  if (bytes > kBlockSize / 4) {
    // Object is more than a quarter of our block size.  Allocate it separately
    // to avoid wasting too much space in leftover bytes.
    char* result = AllocateNewBlock(bytes);
    return result;
  }

  // ��������ڴ�С��1024ʱ����ʱֱ�ӷ���һ��block��������δ��ռ�õ��ڴ档�´�������С�ڴ�ʱ������ֱ�Ӵ�δ��ռ�õ��ڴ�����䡣
  // �����ڴ��˷ѵ����Σ� ��ʣ���ڴ�100bytesʱ����ʱ����200bytes����ʱ��ֱ������һ���µ�block������100bytes�ֽڱ��˷�
  // We waste the remaining space in the current block.
  alloc_ptr_ = AllocateNewBlock(kBlockSize);
  alloc_bytes_remaining_ = kBlockSize;

  char* result = alloc_ptr_;
  alloc_ptr_ += bytes;
  alloc_bytes_remaining_ -= bytes;
  return result;
}
// ���������ڴ档
char* Arena::AllocateAligned(size_t bytes) {
	// ��ȡ��ǰϵͳָ���С
  const int align = (sizeof(void*) > 8) ? sizeof(void*) : 8;
  assert((align & (align-1)) == 0);   // Pointer size should be a power of 2
  // �ж��ǲ���align ��������������align��2��
  // �������ݣ����Զ�align��ȡģ�������ת��Ϊ
  // �ԣ�align - 1�����а�λ��
  size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align-1);
  size_t slop = (current_mod == 0 ? 0 : align - current_mod);
  size_t needed = bytes + slop;
  char* result;
  if (needed <= alloc_bytes_remaining_) {
    result = alloc_ptr_ + slop; // ���ض������ڴ��ַ
    alloc_ptr_ += needed;
    alloc_bytes_remaining_ -= needed;
  } else {
    // AllocateFallback always returned aligned memory
    result = AllocateFallback(bytes);
  }
  assert((reinterpret_cast<uintptr_t>(result) & (align-1)) == 0);
  return result;
}

char* Arena::AllocateNewBlock(size_t block_bytes) {
  char* result = new char[block_bytes];
  blocks_.push_back(result); // ��¼�ѷ�����ڴ�
  memory_usage_.fetch_add(block_bytes + sizeof(char*),
                          std::memory_order_relaxed);
  return result;
}

}  // namespace leveldb
