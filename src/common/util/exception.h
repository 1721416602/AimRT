// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <string>

#include "util/format.h"

namespace aimrt::common::util {

class AimRTException : public std::exception {
 public:
  template <typename... Args>
    requires std::constructible_from<std::string, Args...>
  AimRTException(Args... args)
      : err_msg_(std::forward<Args>(args)...) {}

  ~AimRTException() noexcept override {}

  const char* what() const noexcept override { return err_msg_.c_str(); }

 private:
  std::string err_msg_;
};

}  // namespace aimrt::common::util

//cqmark 如果没有 do { ... } while (0)，在宏中使用 if 语句时，如果宏后面跟着其他语句，可能会导致意想不到的行为。例如：
// if (condition)
//     AIMRT_ASSERT(expr, "Error message", arg);
// else
//     do_something();
// ##可以用于连接两个字符串，也可以用于删除可变参数中的逗号（在没有参数的情况下）

#define AIMRT_ASSERT(__expr__, __fmt__, ...)                                                  \
  do {                                                                                        \
    if (!(__expr__)) [[unlikely]] {                                                           \
      throw aimrt::common::util::AimRTException(::aimrt_fmt::format(__fmt__, ##__VA_ARGS__)); \
    }                                                                                         \
  } while (0)
