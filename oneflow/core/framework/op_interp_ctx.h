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
#ifndef ONEFLOW_CORE_FRAMEWORK_OP_INTERP_CTX_H_
#define ONEFLOW_CORE_FRAMEWORK_OP_INTERP_CTX_H_

#include <string>
#include <vector>

#include "oneflow/core/common/data_type.pb.h"
#include "oneflow/core/common/hash_container.h"
#include "oneflow/core/common/maybe.h"
#include "oneflow/core/common/shape.h"
#include "oneflow/core/common/symbol.h"
#include "oneflow/core/framework/attr_value.h"
#include "oneflow/core/framework/op_attrs.h"
#include "oneflow/core/job/parallel_desc.h"
#include "oneflow/core/job/sbp_parallel.cfg.h"

namespace oneflow {

using user_op::AttrVal;

template<typename T>
using TypedAttrValRef = user_op::TypedAttrValRef<T>;

namespace user_op {
class OpKernelState;
}  // namespace user_op

class OpInterpCtx {
 public:
  virtual ~OpInterpCtx() = default;

  bool HasAttr(const std::string& attr_name) const { return AttrNamesSet().count(attr_name); }

  template<typename T>
  Maybe<const T&> GetAttr(const std::string& attr_name) const {
    const auto& attr_val = JUST(GetAttr(attr_name));
    return AttrValueCast<T>(*JUST(GetAttr(attr_name)));
  }
  virtual Maybe<AttrVal> GetAttr(const std::string& attr_name) const = 0;

  OpAttrs GetAttrs() const { return OpAttrs(this); }

  template<typename T>
  Maybe<void> SetAttr(const std::string& attr_name, const T& attr_value) {
    return SetAttr(attr_name, TypedAttrValRef<T>(&attr_value));
  }

  virtual Maybe<void> SetAttr(const std::string& attr_name, const AttrVal& attr_value) {
    // TODO(hjchen2)
    return Error::RuntimeError() << "There is no attributes to be update.";
  }

  size_t hash_value() const {
    // TODO(hjchen2)
    return 0;
  }

  virtual const HashSet<std::string>& AttrNamesSet() const { return EmptyAttrNamesSet(); }

 public:
  Optional<Symbol<Device>> device;               // for local op
  Optional<Symbol<ParallelDesc>> parallel_desc;  // for consistent op
  Optional<Symbol<cfg::NdSbp>> nd_sbp;           // for consistent op
  Optional<user_op::OpKernelState> state;

 protected:
  OpInterpCtx() = default;

  const HashSet<std::string>& EmptyAttrNamesSet() const {
    static HashSet<std::string> attr_names;
    return attr_names;
  }

  template<typename T>
  Maybe<AttrVal> MakeAttr(const T* attr_val) const {
    return std::dynamic_pointer_cast<AttrVal>(std::make_shared<TypedAttrValRef<T>>(attr_val));
  }
};

class FakeOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    return Error::RuntimeError() << "Should not access attribute for `FakeOpInterpCtx`.";
  }
};

#ifndef DEFINE_OP_INTERP_CTX_CLASS
#define DEFINE_OP_INTERP_CTX_CLASS
#include "oneflow/core/framework/op_interp_ctx_generated.h"
#endif  // DEFINE_OP_INTERP_CTX_CLASS
#undef DEFINE_OP_INTERP_CTX_CLASS

class CastToConsistentOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    if (attr_name == "shape") {
      return MakeAttr(&shape);
    } else if (attr_name == "dtype") {
      return MakeAttr(&dtype);
    } else {
      return Error::RuntimeError() << "CastToConsistent op has no attribute named " << attr_name;
    }
  }

  const HashSet<std::string>& AttrNamesSet() const override {
    static HashSet<std::string> attr_names{"shape", "dtype"};
    return attr_names;
  }

 public:
  Shape shape;
  DataType dtype;
};

class SelectTopNOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    if (attr_name == "top_n") {
      return MakeAttr(&top_n);
    } else {
      return Error::RuntimeError() << "SelectTopN op has no attribute named " << attr_name;
    }
  }

  const HashSet<std::string>& AttrNamesSet() const override {
    static HashSet<std::string> attr_names{"top_n"};
    return attr_names;
  }

 public:
  int32_t top_n;
};

class FeedInputOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    return Error::RuntimeError() << "FeedInput op has no attribute named " << attr_name;
  }
};

class FetchOutputOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    return Error::RuntimeError() << "FetchOutput op has no attribute named " << attr_name;
  }
};

class FeedVariableOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    if (attr_name == "_l2") {
      return MakeAttr(&_l2);
    } else {
      return Error::RuntimeError() << "FeedVariable op has no attribute named " << attr_name;
    }
  }

  const HashSet<std::string>& AttrNamesSet() const override {
    static HashSet<std::string> attr_names{"_l2"};
    return attr_names;
  }

 public:
  double _l2;
};

class ImageDecoderRandomCropResizeOpInterpCtx : public OpInterpCtx {
 public:
  Maybe<AttrVal> GetAttr(const std::string& attr_name) const override {
    if (attr_name == "target_width") {
      return MakeAttr(&target_width);
    } else if (attr_name == "target_height") {
      return MakeAttr(&target_height);
    } else if (attr_name == "num_workers") {
      return MakeAttr(&num_workers);
    } else if (attr_name == "max_num_pixels") {
      return MakeAttr(&max_num_pixels);
    } else if (attr_name == "warmup_size") {
      return MakeAttr(&warmup_size);
    } else if (attr_name == "seed") {
      return MakeAttr(&seed);
    } else if (attr_name == "num_attempts") {
      return MakeAttr(&num_attempts);
    } else if (attr_name == "random_area_min") {
      return MakeAttr(&random_area_min);
    } else if (attr_name == "random_area_max") {
      return MakeAttr(&random_area_max);
    } else if (attr_name == "random_aspect_ratio_min") {
      return MakeAttr(&random_aspect_ratio_min);
    } else if (attr_name == "random_aspect_ratio_max") {
      return MakeAttr(&random_aspect_ratio_max);
    } else {
      return Error::RuntimeError() << "FeedVariable op has no attribute named " << attr_name;
    }
  }

  const HashSet<std::string>& AttrNamesSet() const override {
    static HashSet<std::string> attr_names{"target_width",
                                           "target_height",
                                           "num_workers",
                                           "max_num_pixels",
                                           "warmup_size",
                                           "seed",
                                           "num_attempts",
                                           "random_area_min",
                                           "random_area_max",
                                           "random_aspect_ratio_min",
                                           "random_aspect_ratio_max"};
    return attr_names;
  }

 public:
  int64_t target_width;
  int64_t target_height;
  int64_t num_workers;
  int64_t max_num_pixels;
  int64_t warmup_size;
  int64_t seed;
  int64_t num_attempts;
  float random_area_min;
  float random_area_max;
  float random_aspect_ratio_min;
  float random_aspect_ratio_max;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_FRAMEWORK_OP_INTERP_CTX_H_
