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
#include "oneflow/core/graph/boxing/b21_sub_task_graph_builder.h"
#include "oneflow/core/graph/boxing/sub_task_graph_builder_util.h"

namespace oneflow {

Maybe<SubTskGphBuilderStatus> B21SubTskGphBuilder::Build(
    SubTskGphBuilderCtx* ctx, const std::vector<TaskNode*>& sorted_in_tasks,
    std::vector<TaskNode*>* sorted_out_tasks,
    std::vector<std::vector<TaskNode*>>* sorted_ctrl_tasks, const ParallelDesc& in_parallel_desc,
    const ParallelDesc& out_parallel_desc, const LogicalBlobId& lbi,
    const BlobDesc& logical_blob_desc, const cfg::SbpParallel& in_sbp_parallel,
    const cfg::SbpParallel& out_sbp_parallel, const Shape& time_shape) const {
  if ((in_parallel_desc.parallel_num() == 1 || in_sbp_parallel.has_broadcast_parallel())
      && out_parallel_desc.parallel_num() == 1) {
    const int64_t out_parallel_id = 0;
    const int64_t nearest_in_parallel_id = SubTskGphBuilderUtil::FindNearestSrcParallelId(
        in_parallel_desc, out_parallel_desc, out_parallel_id);
    sorted_ctrl_tasks->resize(1);
    FOR_RANGE(int64_t, i, 0, in_parallel_desc.parallel_num()) {
      TaskNode* in_node = sorted_in_tasks.at(i);
      if (i == nearest_in_parallel_id) {
        TaskNode* proxy =
            ctx->task_graph()->GetProxyNode(in_node, lbi, out_parallel_desc, out_parallel_id);
        sorted_out_tasks->emplace_back(proxy);
      } else {
        sorted_ctrl_tasks->at(0).emplace_back(in_node);
      }
    }
    return TRY(BuildSubTskGphBuilderStatus("B21SubTskGphBuilder", ""));
  } else {
    return Error::BoxingNotSupportedError();
  }
}

}  // namespace oneflow
