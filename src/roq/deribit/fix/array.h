/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include "roq/deribit/fix/buffer.h"

namespace roq {
namespace deribit {
namespace fix {

template <typename T>
class Array final {
 public:
  inline Array(Buffer& buffer, T& array)
      : _buffer(buffer),
        _array(array) {
    _buffer.initialize(_array);
    _buffer.resize(_array);
  }

  Array(Array&) = delete;
  void operator=(Array&) = delete;

  inline ~Array() {
    _buffer.finalize(_array);
  }

  inline Array& operator++() {
    ++_array.length;
    _buffer.resize(_array);
    return *this;
  }

  inline auto& next() {
    return _array.items[_array.length];
  }

 private:
  Buffer& _buffer;
  T& _array;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
