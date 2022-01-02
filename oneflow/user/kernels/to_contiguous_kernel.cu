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
#include <type_traits>
#include "oneflow/core/common/device_type.pb.h"
#include "oneflow/core/graph/task_node.h"
#include "oneflow/user/kernels/to_contiguous_kernel.h"
#include "oneflow/core/ep/cuda/cuda_stream.h"
#include "oneflow/core/cuda/elementwise.cuh"

namespace oneflow {

namespace {

constexpr int kBlockSize = cuda::elementwise::kBlockSize;
__constant__ int64_t in_stride_[SHAPE_MAX_AXIS_SIZE] = {1};
__constant__ int64_t out_stride_[SHAPE_MAX_AXIS_SIZE] = {1};

int GetMinThreadNum(int64_t elem_cnt) { return std::min<int64_t>(elem_cnt, kBlockSize); }

int GetNumBlocks(int64_t elem_cnt) {
  int num_blocks = 0;
  OF_CUDA_CHECK(cuda::elementwise::GetNumBlocks(elem_cnt, &num_blocks));
  return num_blocks;
}

template<typename IndexType>
__device__ IndexType compute_index(IndexType out_offset, const size_t ndim) {
  IndexType in_offset = 0;
  IndexType remaining = out_offset;

#pragma unroll
  for (size_t i = 0; i < ndim; ++i) {
    const IndexType idx = remaining / out_stride_[i];
    remaining = remaining - idx * out_stride_[i];
    in_offset = in_offset + idx * in_stride_[i];
  }

  return in_offset;
}

template<typename T, typename IndexType, size_t pack_size>
__global__ void ToContiguousForwardGpu(IndexType count, size_t ndim, const T* in_dptr,
                                       T* out_dptr) {
  for (IndexType out_idx = blockIdx.x * blockDim.x + threadIdx.x, step = blockDim.x * gridDim.x;
       out_idx < count; out_idx += step) {
    IndexType in_idx = compute_index<IndexType>(out_idx, ndim);
    out_dptr[out_idx] = in_dptr[in_idx];
  }
}

template<typename T, typename IndexType, size_t pack_size>
void LaunchToContiguousKernel(ep::Stream* stream, IndexType count, const size_t ndim,
                              IndexType block_size, const std::vector<int64_t>& in_stride,
                              const StrideVector& out_stride, const char* in_dptr, char* out_dptr) {
  const int num_blocks = GetNumBlocks(count);
  const int num_threads = GetMinThreadNum(count);
  OF_CUDA_CHECK(
      cudaMemcpyToSymbol(in_stride_, in_stride.data(), in_stride.size() * sizeof(int64_t)));
  OF_CUDA_CHECK(
      cudaMemcpyToSymbol(out_stride_, out_stride.data(), out_stride.size() * sizeof(int64_t)));

  ToContiguousForwardGpu<T, IndexType, pack_size>
      <<<num_blocks, num_threads, 0, stream->As<ep::CudaStream>()->cuda_stream()>>>(
          count, ndim, reinterpret_cast<const T*>(in_dptr), reinterpret_cast<T*>(out_dptr));
}

}  // namespace

template<typename T>
struct ToContiguousUtil<DeviceType::kCUDA, T> : ToContiguousUtilBase {
  using ToContiguousUtilBase::ToContiguousUtilBase;
  static constexpr size_t dsize = sizeof(T);
  void operator()() {
    int constant_memory_size = 0;
    // get device constant memory capacity, for RTX 2080, constant_memory_size is 65536(64kb)
    cudaDeviceGetAttribute(&constant_memory_size, cudaDevAttrTotalConstantMemory, 0);
    const size_t ndims = contiguous_dim + 1;
    if (ndims == 0) {
      // 0-dim tensor
      OF_CUDA_CHECK(cudaMemcpyAsync(out_dptr, in_dptr, block_size * dsize, cudaMemcpyDeviceToDevice,
                                    stream->As<ep::CudaStream>()->cuda_stream()));
    } else {
      bool is_same = true;
      for (int64_t i = contiguous_dim; i != -1; --i) {
        if (out_stride[i] != in_stride[i]) {
          is_same = false;
          break;
        }
      }
      if (is_same) {
        // if input tensor's strides equals to output's, than just copy one memory-contiguous tensor
        OF_CUDA_CHECK(cudaMemcpyAsync(out_dptr + out_offset * dsize, in_dptr + in_offset * dsize,
                                      element_count * dsize, cudaMemcpyDeviceToDevice,
                                      stream->As<ep::CudaStream>()->cuda_stream()));
      } else {
        constexpr size_t pack_size = cuda::elementwise::PackSize<T>();
        if (element_count < GetMaxVal<int32_t>()) {
          LaunchToContiguousKernel<T, int32_t, pack_size>(stream, element_count, ndims, block_size,
                                                          in_stride, out_stride, in_dptr, out_dptr);
        } else {
          LaunchToContiguousKernel<T, int64_t, pack_size>(stream, element_count, ndims, block_size,
                                                          in_stride, out_stride, in_dptr, out_dptr);
        }
      }
    }
  }
};

#define INSTANTIATE_TO_CONTIGUOUS_UTILS_FOR_CUDA(T) \
  template struct ToContiguousUtil<DeviceType::kCUDA, T>;
OF_PP_FOR_EACH_TUPLE(INSTANTIATE_TO_CONTIGUOUS_UTILS_FOR_CUDA,
                     TO_CONTIGUOUS_TYPES TO_CONTIGUOUS_CUDA_SPECIAL_TYPE)

}  // namespace oneflow
