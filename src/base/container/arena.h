// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MOZC_BASE_CONTAINER_ARENA_H_
#define MOZC_BASE_CONTAINER_ARENA_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/check.h"

// A simple arena allocator for the given type T.
template <class T>
class Arena final {
 public:
  explicit Arena(size_t chunk_size) : chunk_size_(chunk_size) {
    CHECK_GT(chunk_size, 0);
  }

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  Arena(Arena&& other) noexcept
      : chunks_(std::move(other.chunks_)),
        next_in_chunk_(std::exchange(other.next_in_chunk_, 0)),
        chunk_size_(other.chunk_size_) {}

  Arena& operator=(Arena&& other) noexcept {
    if (this != &other) {
      Clear();
      chunks_ = std::move(other.chunks_);
      next_in_chunk_ = std::exchange(other.next_in_chunk_, 0);
      chunk_size_ = other.chunk_size_;
    }
    return *this;
  }

  ~Arena() { Clear(); }

  // Allocates memory for a new T and constructs it in place with the given
  // arguments.
  template <class... Args>
  [[nodiscard]] T* absl_nonnull Alloc(Args&&... args) {
    if (chunks_.empty() || next_in_chunk_ >= chunk_size_) [[unlikely]] {
      chunks_.push_back(std::allocator<T>{}.allocate(chunk_size_));
      next_in_chunk_ = 0;
    }
    return std::construct_at(chunks_.back() + next_in_chunk_++,
                             std::forward<Args>(args)...);
  }

  // Frees all allocated memory and destroys all objects.
  void Clear() {
    if (chunks_.empty()) {
      return;
    }
    // Clear the last chunk.
    destroy_n_reverse(chunks_.back(), next_in_chunk_);
    std::allocator<T>{}.deallocate(chunks_.back(), chunk_size_);
    chunks_.pop_back();
    // Clear the remaining chunks.
    while (!chunks_.empty()) {
      destroy_n_reverse(chunks_.back(), chunk_size_);
      std::allocator<T>{}.deallocate(chunks_.back(), chunk_size_);
      chunks_.pop_back();
    }
    next_in_chunk_ = 0;
  }

 private:
  // Like std::destroy_n, but destroys in reverse order.
  void destroy_n_reverse(T* absl_nonnull first, size_t n) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      while (n > 0) {
        std::destroy_at(first + --n);
      }
    }
  }

  std::vector<T* absl_nonnull> chunks_;
  size_t next_in_chunk_ = 0;
  size_t chunk_size_;
};

// A wrapper around Arena that reuses previously freed objects.
template <class T>
class ObjectPool final {
 public:
  explicit ObjectPool(size_t chunk_size) : arena_(chunk_size) {}

  // Allocates memory for a new T and constructs it in place with the given
  // arguments.
  template <class... Args>
  [[nodiscard]] T* absl_nonnull Alloc(Args&&... args) {
    if (released_.empty()) {
      return arena_.Alloc(std::forward<Args>(args)...);
    }
    T* ptr = released_.back();
    released_.pop_back();
    if constexpr (!std::is_trivially_destructible_v<T>) {
      std::destroy_at(ptr);
    }
    return std::construct_at(ptr, std::forward<Args>(args)...);
  }

  // Returns the given object to the pool for reuse. Note that the destructor
  // won't run until the memory is actually reused.
  void Release(T* absl_nonnull ptr) { released_.push_back(ptr); }

  // Returns the number of reusable object slots.
  size_t NumReusable() const { return released_.size(); }

  // Frees all allocated memory and destroys all objects.
  void Clear() {
    arena_.Clear();
    released_.clear();
  }

 private:
  Arena<T> arena_;
  std::vector<T* absl_nonnull> released_;
};

#endif  // MOZC_BASE_CONTAINER_ARENA_H_
