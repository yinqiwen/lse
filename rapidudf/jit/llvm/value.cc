/*
** BSD 3-Clause License
**
** Copyright (c) 2023, qiyingwang <qiyingwang@tencent.com>, the respective contributors, as shown by the AUTHORS file.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** * Redistributions of source code must retain the above copyright notice, this
** list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright notice,
** this list of conditions and the following disclaimer in the documentation
** and/or other materials provided with the distribution.
**
** * Neither the name of the copyright holder nor the names of its
** contributors may be used to endorse or promote products derived from
** this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "rapidudf/jit/llvm/value.h"
#include <fmt/core.h>
#include <llvm/IR/Use.h>
#include "rapidudf/jit/llvm/jit.h"
#include "rapidudf/jit/llvm/type.h"
#include "rapidudf/log/log.h"
#include "rapidudf/meta/optype.h"

namespace rapidudf {
namespace llvm {
Value::Value(Private, DType dtype, JitCompiler* c, ::llvm::Value* val, ::llvm::Type* t)
    : dtype_(dtype), compiler_(c), val_(val), type_(t) {
  ir_builder_ = compiler_->GetIRBuilder();
}
::llvm::Value* Value::GetValue() {
  if (type_ != nullptr) {
    return ir_builder_->CreateLoad(type_, val_);
  }
  return val_;
}
absl::Status Value::CopyFrom(ValuePtr other) {
  if (!dtype_.IsVoid()) {
    if (dtype_ != other->dtype_) {
      return absl::InvalidArgumentError(
          fmt::format("Can not copy from dtype:{} while current dtype:{}", other->dtype_, dtype_));
    }
  } else {
    type_ = get_type(ir_builder_->getContext(), other->GetDType());
    if (type_ == nullptr) {
      return absl::InvalidArgumentError(fmt::format("Can not alloca for dtype:{}", other->GetDType()));
    }
    val_ = ir_builder_->CreateAlloca(type_);
  }
  dtype_ = other->dtype_;
  if (type_ != nullptr) {
    ir_builder_->CreateStore(other->val_, val_);
  } else {
    val_ = other->val_;
  }
  return absl::OkStatus();
}
ValuePtr Value::Select(ValuePtr true_val, ValuePtr false_val) {
  if (true_val->dtype_ != false_val->dtype_) {
    RUDF_ERROR("Can NOT select since true_val dtype:{} is not eqaul with false_val dtype:{}", true_val->GetDType(),
               false_val->GetDType());
    return {};
  }
  auto new_val = ir_builder_->CreateSelect(val_, true_val->GetValue(), false_val->GetValue());
  return New(true_val->GetDType(), compiler_, new_val);
}
}  // namespace llvm
}  // namespace rapidudf