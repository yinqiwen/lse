/*
 * Copyright (c) 2024 yinqiwen yinqiwen@gmail.com. All rights reserved.
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

#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>
#include "boost/thread/tss.hpp"

#include "rapidudf/memory/allocator.h"
#include "rapidudf/memory/folly_arena.h"
#include "rapidudf/memory/leveldb_arena.h"

namespace rapidudf {

class Arena;
class ThreadCachedArena;
template <typename T>
using ArenaAllocator = CxxAllocatorAdaptor<T, Arena>;

template <typename T>
using ThreadCachedArenaAllocator = CxxAllocatorAdaptor<T, ThreadCachedArena>;

template <typename T>
struct ArenaObjDeleter {
  void operator()([[maybe_unused]] T* ptr) const {
    if constexpr (std::is_trivially_destructible_v<T>) {
      // do nothing
    } else {
      ptr->~T();
    }
  }
};

template <typename T>
using ArenaObjPtr = std::unique_ptr<T, ArenaObjDeleter<T>>;

template <typename T, typename Alloc, typename = void, typename... Args>
struct HasAllocatorConstructorImpl : std::false_type {};

template <typename T, typename Alloc, typename... Args>
struct HasAllocatorConstructorImpl<T, Alloc, std::void_t<decltype(T(std::declval<Args>()..., std::declval<Alloc>()))>,
                                   Args...> : std::true_type {};

template <typename T, typename Alloc, typename... Args>
struct HasAllocatorConstructor : HasAllocatorConstructorImpl<T, Alloc, void, Args...> {};

template <typename T, typename Alloc, typename... Args>
constexpr bool has_allocator_constructor_v = HasAllocatorConstructor<T, Alloc, Args...>::value;

class Arena {
 public:
  using CleanupFunc = std::function<void()>;

  Arena() { arena_ = std::make_unique<folly::Arena>(); }

  template <typename T, typename... Args>
  ArenaObjPtr<T> New(Args&&... args) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      static_assert(sizeof(T) == -1, "Arena class MUST be trivially destructible!");
    }
    ArenaAllocator<T> allocator(*this);
    T* memory = allocator.allocate(1);

    if constexpr (has_allocator_constructor_v<T, ArenaAllocator<T>, Args...>) {
      new (memory) T(std::forward<Args>(args)..., allocator);
    } else {
      new (memory) T(std::forward<Args>(args)...);
    }

    ArenaObjDeleter<T> d;
    return ArenaObjPtr<T>(memory, d);
  }

  uint8_t* Allocate(size_t n) { return reinterpret_cast<uint8_t*>(arena_->allocate(n)); }
  void Reset() { arena_->clear(); }

  size_t MemoryUsage() const { return arena_->bytesUsed(); }

  void deallocate(void* /* p */, size_t = 0) {
    // Deallocate? Never!
  }
  void* allocate(size_t size) { return Allocate(size); }

  template <typename U>
  inline void destroy([[maybe_unused]] U* p) {
    if constexpr (std::is_trivially_destructible_v<U>) {
      // do nothing
    } else {
      p->~U();
    }
  }

 private:
  std::unique_ptr<folly::Arena> arena_;
};

class ThreadCachedArena {
 public:
  ThreadCachedArena();

  template <typename T, typename... Args>
  ArenaObjPtr<T> New(Args&&... args) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      static_assert(sizeof(T) == -1, "Arena class MUST be trivially destructible!");
    }
    ThreadCachedArenaAllocator<T> allocator(*this);
    T* memory = allocator.allocate(1);

    if constexpr (has_allocator_constructor_v<T, ThreadCachedArenaAllocator<T>, Args...>) {
      new (memory) T(std::forward<Args>(args)..., allocator);
    } else {
      new (memory) T(std::forward<Args>(args)...);
    }

    ArenaObjDeleter<T> d;
    return ArenaObjPtr<T>(memory, d);
  }
  uint8_t* Allocate(size_t n) { return GetArena()->Allocate(n); }

  size_t MemoryUsage() const;

  void Reset();

  void deallocate(void* /* p */, size_t = 0) {
    // Deallocate? Never!
  }
  void* allocate(size_t size) { return Allocate(size); }

  template <typename U>
  inline void destroy([[maybe_unused]] U* p) {
    if constexpr (std::is_trivially_destructible_v<U>) {
      // do nothing
    } else {
      p->~U();
    }
  }

  ~ThreadCachedArena();

 private:
  Arena* GetArena();

  boost::thread_specific_ptr<Arena> arena_;
  std::vector<Arena*> all_arenas_;
  mutable std::mutex threads_mutex_;
};

}  // namespace rapidudf