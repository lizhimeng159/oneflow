/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ONEFLOW_CORE_VECTORIZED_VEC_BINARY_MATH_H_
#define ONEFLOW_CORE_VECTORIZED_VEC_BINARY_MATH_H_
#include <iostream>
#include "oneflow/core/thread/thread_manager.h"
#include "oneflow/core/vectorized/vec.h"

namespace oneflow {

template<typename T>
inline void vec_binary_add(const T* x, const T* y, T* out, size_t len) {
  vectorized_init();

  MultiThreadVecLoop(
      len, [=](size_t begin, size_t end) { VecFunc<T>::add_func(begin, end, x, y, out); });
}

}  // namespace oneflow
#endif
