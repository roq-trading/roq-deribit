/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include "roq/platform.h"

#include "roq/core/utility.h"

namespace roq {
namespace deribit {
namespace fix {

class Buffer final : NonCopyable {
 public:
  explicit Buffer(std::vector<std::byte>& buffer)
      : _buffer(buffer) {
  }

  template <typename T>
  void initialize(T& array) {
    assert(array.items == nullptr);
    assert(array.length == 0);
    array.items = reinterpret_cast<decltype(array.items)>(get());
  }

  template <typename T>
  void resize(T& array) {
    assert(array.items != nullptr);
    ensure(array.items + array.length + 1);
    new (array.items + array.length)
      std::remove_pointer<decltype(array.items)> {};
  }

  template <typename T>
  void finalize(T& array) {
    assert(array.items != nullptr);
    commit(array.items + array.length);
  }

 protected:
  auto get() {
    return _buffer.data() +
      core::round_up<cache_line_size()>(_next);
  }
  void ensure(void *ptr) {
    auto offset = reinterpret_cast<std::byte *>(ptr) -
      _buffer.data();
    assert(offset > 0);
    if (_buffer.size() < static_cast<size_t>(offset))
      throw std::range_error("not enough space");
  }
  auto commit(void *ptr) {
    auto offset = reinterpret_cast<std::byte *>(ptr) -
      _buffer.data();
    assert(offset > 0);
    if (_buffer.size() < static_cast<size_t>(offset))
      throw std::range_error("not enough space");
    _next = static_cast<size_t>(offset);
  }

 private:
  std::vector<std::byte>& _buffer;
  size_t _next = 0;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
