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
#include "rapidudf/meta/function.h"
#include "rapidudf/log/log.h"
#include "rapidudf/meta/optype.h"

namespace rapidudf {
using FuncRegMap = std::unordered_map<std::string, FunctionDesc>;
static std::unique_ptr<FuncRegMap> g_regs = nullptr;

// static inline uint32_t allgin_n(uint32_t x, uint32_t n) { return (x + n - 1) & ~(n - 1); }

std::string GetFunctionName(std::string_view op, DType dtype) {
  std::string fname(op);
  if (dtype.IsSimdVector()) {
    fname = fname + "_" + dtype.Elem().GetTypeString();
    return std::string(FunctionFactory::kSimdVectorFuncPrefix) + "_" + fname;
  } else {
    fname = fname + "_" + dtype.Elem().GetTypeString();
    return fname;
  }
}
std::string GetFunctionName(std::string_view op, DType dtype0, DType dtype1) {
  std::string fname(op);
  if (dtype0.IsSimdVector() || dtype1.IsSimdVector()) {
    fname = fname + "_" + dtype0.Elem().GetTypeString() + "_" + dtype1.Elem().GetTypeString();
    return std::string(FunctionFactory::kSimdVectorFuncPrefix) + "_" + fname;
  } else {
    fname = fname + "_" + dtype0.Elem().GetTypeString() + "_" + dtype1.Elem().GetTypeString();
    return fname;
  }
}

std::string GetMemberFuncName(DType dtype, const std::string& member) {
  std::string fname = fmt::format("{}_{}", dtype.GetTypeString(), member);
  return fname;
}

std::string GetFunctionName(OpToken op, DType dtype) { return GetFunctionName(kOpTokenStrs[op], dtype); }
std::string GetFunctionName(OpToken op, DType dtype0, DType dtype1) {
  return GetFunctionName(kOpTokenStrs[op], dtype0, dtype1);
}
// std::string GetFunctionName(OpToken op, DType a, DType b) { return GetFunctionName(kOpTokenStrs[op], a, b); }
// std::string GetFunctionName(OpToken op, DType a, DType b, DType c) {
//   return GetFunctionName(kOpTokenStrs[op], a, b, c);
// }
// std::string GetFunctionName(OpToken op, const std::vector<DType>& arg_dtypes) {
//   switch (op) {
//     case OP_SORT:
//     case OP_SELECT:
//     case OP_TOPK:
//     case OP_IOTA:
//     case OP_ARG_SORT:
//     case OP_ARG_SELECT:
//     case OP_FILTER:
//     case OP_GATHER: {
//       if (arg_dtypes.size() > 0) {
//         return GetFunctionName(op, arg_dtypes[0]);
//       }
//     }
//     case OP_SORT_KV:
//     case OP_SELECT_KV:
//     case OP_TOPK_KV: {
//       if (arg_dtypes.size() > 1) {
//         return GetFunctionName(op, arg_dtypes[0], arg_dtypes[1]);
//       }
//     }
//     default: {
//       break;
//     }
//   }
//   return GetFunctionName(kOpTokenStrs[op], arg_dtypes);
// }

void FunctionDesc::Init() {
  for (size_t i = 0; i < arg_types.size(); i++) {
    if (arg_types[i].IsContextPtr()) {
      if (context_arg_idx != -1) {
        RUDF_ERROR("Function:{} has more than ONE arg type is ContextPtr({}), the first is at:{}", i, context_arg_idx);
      } else {
        context_arg_idx = static_cast<int>(i);
      }
    }
  }
}
bool FunctionDesc::PassArgByValue(size_t argno) const {
  if (argno >= arg_types.size()) {
    return false;
  }
  // x86-64 linux
  uint32_t used_param_registers = 0;
  uint32_t total_param_registers = 6;
  for (size_t i = 0; i <= argno; i++) {
    uint32_t request_param_registers = 0;
    if (arg_types[i].IsPtr() || arg_types[i].IsInteger() || arg_types[i].IsBit()) {
      request_param_registers = 1;
    } else if (arg_types[i].IsAbslSpan() || arg_types[i].IsStringView() || arg_types[i].IsStdStringView() ||
               arg_types[i].IsSimdVector()) {
      request_param_registers = 2;
    }
    used_param_registers += request_param_registers;
  }
  if (arg_types[argno].IsSimdVector() || arg_types[argno].IsAbslSpan() || arg_types[argno].IsStringView() ||
      arg_types[argno].IsStdStringView()) {
    if (used_param_registers > total_param_registers) {
      return true;
    }
  }
  return false;
}

bool FunctionDesc::ValidateArgs(const std::vector<DType>& ts) const {
  if (arg_types.size() != ts.size()) {
    return false;
  }
  for (size_t i = 0; i < ts.size(); i++) {
    if (!ts[i].CanCastTo(arg_types[i])) {
      return false;
    }
  }
  return true;
}

bool FunctionDesc::CompareSignature(DType rtype, const std::vector<DType>& validate_args_types) const {
  if (return_type != rtype) {
    return false;
  }
  if (arg_types.size() != validate_args_types.size()) {
    return false;
  }
  for (size_t i = 0; i < arg_types.size(); i++) {
    if (arg_types[i] != validate_args_types[i]) {
      return false;
    }
  }
  return true;
}

bool FunctionFactory::Register(FunctionDesc&& desc) {
  desc.Init();
  if (!g_regs) {
    g_regs = std::make_unique<FuncRegMap>();
  }
  if (g_regs->count(desc.name) > 0) {
    RUDF_CRITICAL("Duplicate func name:{}", desc.name);
    return false;
  }
  // RUDF_DEBUG("Registe function:{}", desc.name);
  return g_regs->emplace(desc.name, desc).second;
}
const FunctionDesc* FunctionFactory::GetFunction(const std::string& name) {
  if (!g_regs) {
    return nullptr;
  }
  auto found = g_regs->find(name);
  if (found == g_regs->end()) {
    return nullptr;
  }
  return &(found->second);
}
}  // namespace rapidudf