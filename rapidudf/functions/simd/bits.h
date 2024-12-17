/*
 * Copyright (c) 2024 qiyingwang <qiyingwang@tencent.com>. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "rapidudf/context/context.h"
#include "rapidudf/functions/simd/vector_op.h"
#include "rapidudf/meta/optype.h"
#include "rapidudf/types/vector.h"

namespace rapidudf {
namespace functions {
void simd_vector_bits_not(Vector<Bit> src, Vector<Bit> dst);
void simd_vector_bits_and(Vector<Bit> left, Vector<Bit> right, Vector<Bit> dst);
void simd_vector_bits_or(Vector<Bit> left, Vector<Bit> right, Vector<Bit> dst);
void simd_vector_bits_xor(Vector<Bit> left, Vector<Bit> right, Vector<Bit> dst);
size_t simd_vector_bits_count_true(Vector<Bit> src);
}  // namespace functions
}  // namespace rapidudf