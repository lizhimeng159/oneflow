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
#include "oneflow/core/common/util.h"
#include "oneflow/core/intrusive/flat_msg_view.h"
#include "oneflow/core/rpc/include/base.h"
#include "oneflow/core/vm/control_stream_type.h"
#include "oneflow/core/vm/host_stream_type.h"
#include "oneflow/core/vm/instruction_type.h"
#include "oneflow/core/vm/instruction.h"
#include "oneflow/core/vm/instruction_operand.h"
#include "oneflow/core/vm/virtual_machine_engine.h"
#include "oneflow/core/vm/no_arg_cb_phy_instr_operand.h"
#include "oneflow/core/control/global_process_ctx.h"

namespace oneflow {
namespace vm {

class RankFrontSeqCallbackInstructionType : public InstructionType {
 public:
  RankFrontSeqCallbackInstructionType() = default;
  virtual ~RankFrontSeqCallbackInstructionType() override = default;

  bool IsFrontSequential() const override { return true; }

 protected:
  // clang-format off
  FLAT_MSG_VIEW_BEGIN(RankFrontSeqCallbackInstrOperand);
    FLAT_MSG_VIEW_DEFINE_PATTERN(int64_t, process_rank);
  FLAT_MSG_VIEW_END(RankFrontSeqCallbackInstrOperand);
  // clang-format on

  void Run(const InstructionMsg& instr_msg) const {
    FlatMsgView<RankFrontSeqCallbackInstrOperand> args(instr_msg.operand());
    const auto& phy_instr_operand = instr_msg.phy_instr_operand();
    if (args->process_rank() == GlobalProcessCtx::Rank()) {
      CHECK(static_cast<bool>(phy_instr_operand));
      const auto* ptr = dynamic_cast<const NoArgCbPhyInstrOperand*>(phy_instr_operand.get());
      CHECK_NOTNULL(ptr);
      ptr->callback()();
    } else {
      CHECK(!static_cast<bool>(phy_instr_operand));
    }
  }
};

class ComputeRankFrontSeqCallbackInstructionType final
    : public RankFrontSeqCallbackInstructionType {
 public:
  ComputeRankFrontSeqCallbackInstructionType() = default;
  ~ComputeRankFrontSeqCallbackInstructionType() override = default;

  using stream_type = HostStreamType;

  void Infer(Instruction* instruction) const override {}
  void Compute(Instruction* instruction) const override { Run(instruction->instr_msg()); }
};
COMMAND(RegisterInstructionType<ComputeRankFrontSeqCallbackInstructionType>(
    "ComputeRankFrontSeqCallback"));

class CtrlComputeRankFrontSeqCallbackInstructionType final
    : public RankFrontSeqCallbackInstructionType {
 public:
  CtrlComputeRankFrontSeqCallbackInstructionType() = default;
  ~CtrlComputeRankFrontSeqCallbackInstructionType() override = default;

  using stream_type = ControlStreamType;

  void Infer(VirtualMachineEngine*, InstructionMsg* instr_msg) const override {}
  void Compute(VirtualMachineEngine*, InstructionMsg* instr_msg) const override { Run(*instr_msg); }
  void Infer(Instruction* instruction) const override {}
  void Compute(Instruction* instruction) const override { UNIMPLEMENTED(); }
};
COMMAND(RegisterInstructionType<CtrlComputeRankFrontSeqCallbackInstructionType>(
    "CtrlComputeRankFrontSeqCallback"));

class GlobalFrontSeqBarrierInstructionType : public InstructionType {
 public:
  GlobalFrontSeqBarrierInstructionType() = default;
  virtual ~GlobalFrontSeqBarrierInstructionType() override = default;

  using stream_type = HostStreamType;

  virtual bool IsFrontSequential() const override { return true; }
};

class ComputeGlobalFrontSeqBarrierInstructionType final
    : public GlobalFrontSeqBarrierInstructionType {
 public:
  ComputeGlobalFrontSeqBarrierInstructionType() = default;
  ~ComputeGlobalFrontSeqBarrierInstructionType() override = default;

  void Infer(Instruction* instruction) const override {}
  void Compute(Instruction* instruction) const override { OF_ENV_BARRIER(); }
};
COMMAND(RegisterInstructionType<ComputeGlobalFrontSeqBarrierInstructionType>(
    "ComputeGlobalFrontSeqBarrier"));

}  // namespace vm
}  // namespace oneflow
