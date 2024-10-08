// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <string_view>
#include <thread>
#include <vector>

namespace aimrt::runtime::core::util {

void SetNameForCurrentThread(std::string_view thread_name);

void BindCpuForCurrentThread(const std::vector<uint32_t>& cpu_set);

void SetCpuSchedForCurrentThread(std::string_view sched);

}  // namespace aimrt::runtime::core::util
