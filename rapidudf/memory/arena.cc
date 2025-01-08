/*
 * Copyright (c) 2025 qiyingwang <qiyingwang@tencent.com>. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "rapidudf/memory/arena.h"
#include <mutex>
#include "rapidudf/log/log.h"
namespace rapidudf {
ThreadCachedArena::ThreadCachedArena()
    : arena_([](Arena* p) {
        // do nothing
      }) {}
Arena* ThreadCachedArena::GetArena() {
  Arena* arena = arena_.get();
  if (arena == nullptr) {
    arena = new Arena;
    arena_.reset(arena);
    std::lock_guard<std::mutex> guard(threads_mutex_);
    all_arenas_.emplace_back(arena);
  }
  return arena;
}

void ThreadCachedArena::Reset() {
  std::lock_guard<std::mutex> guard(threads_mutex_);
  for (auto arena : all_arenas_) {
    arena->Reset();
  }
}

size_t ThreadCachedArena::MemoryUsage() const {
  size_t total = 0;
  std::lock_guard<std::mutex> guard(threads_mutex_);
  for (auto arena : all_arenas_) {
    total += arena->MemoryUsage();
  }
  return total;
}

ThreadCachedArena::~ThreadCachedArena() {
  for (auto arena : all_arenas_) {
    delete arena;
  }
}
}  // namespace rapidudf