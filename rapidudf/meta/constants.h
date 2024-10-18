/*
 * Copyright (c) 2024 yinqiwen yinqiwen@gmail.com. All rights reserved.
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
#include <array>
#include <string_view>
namespace rapidudf {
static constexpr size_t kConstantCount = 2;
static constexpr std::array<std::string_view, kConstantCount> kConstantNames = {"pi", "e"};
static constexpr std::array<double, kConstantCount> kConstantValues = {3.141592653589793238462643,
                                                                       2.718281828459045235360287};
}  // namespace rapidudf