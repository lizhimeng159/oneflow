#ifndef OPERATOR_MULTINOMIAL_LOGISTIC_LOSS_OP_H_
#define OPERATOR_MULTINOMIAL_LOGISTIC_LOSS_OP_H_

#include "operator/operator.h"

namespace oneflow {

// MLLoss = MultinomialLogisticLoss

class MultinomialLogisticLossOp : public UserOperator {
 public:
  OF_DISALLOW_COPY_AND_MOVE(MultinomialLogisticLossOp);
  MultinomialLogisticLossOp() = default;
  ~MultinomialLogisticLossOp() = default;

  std::string GetValueFromPbOpConf(const std::string& k) const override;
  void InitFromOpConf(const OperatorConf& op_conf) override;
  bool IsLossOp() const override { return true; }
  
  void InferShape4ObAndDtbFromIb() const override { TODO(); }
  void InferShape4ModelTmpBlob(ParallelPolicy, uint64_t parallel_id) const override {
    TODO();
  }
  void InferShape4ModelDiffBlob(ParallelPolicy, uint64_t parallel_id) const override {
    TODO();
  }

 private:

};

} // namespace oneflow

#endif // OPERATOR_MULTINOMIAL_LOGISTIC_LOSS_OP_H_
