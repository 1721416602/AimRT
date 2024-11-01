// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

namespace aimrt::common::util {
// The following source code is from trpc-cpp.
// Copied and modified from
// https://github.com/trpc-group/trpc-cpp/blob/v1.2.0/trpc/util/deferred.h

class Deferred {
 public:
  Deferred() = default;

//cqmark 确保当 F 不是 Deferred 类型时才允许调用这个构造函数。这避免了 Deferred 移动构造函数和这个模板构造函数发生冲突。
  template <class F>
    requires(!std::same_as<std::decay_t<F>, Deferred>)  // Avoid hide Deferred(Deferred&&)
  explicit Deferred(F&& f) : action_(std::forward<F>(f)) {}

  Deferred(Deferred&&) noexcept = default;
  Deferred& operator=(Deferred&&) noexcept = default;

  ~Deferred() {
    if (action_) {
      action_();
    }
  }

//cqmark !!用于确保表达式返回的是布尔值类型。
//用法为Deferred a; if(a){}

  explicit operator bool() const noexcept { return !!action_; }

  void Dismiss() noexcept { action_ = nullptr; }

 private:
  std::function<void()> action_;
};
// End of the source code from trpc-cpp.

}  // namespace aimrt::common::util
