// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

namespace aimrt::common::util {
// cqmark 继承自std::runtime_error方便抛出异常
class BlockQueueStoppedException : public std::runtime_error {
 public:
  BlockQueueStoppedException() : std::runtime_error("BlockQueue is stopped") {}
};

template <class T>
class BlockQueue {
 public:
  BlockQueue() = default;
  ~BlockQueue() { Stop(); }

  BlockQueue(const BlockQueue &) = delete;
  BlockQueue &operator=(const BlockQueue &) = delete;
  //cqmark 右值引用和const不兼容
  void Enqueue(const T &item) {
    std::unique_lock<std::mutex> lck(mutex_);
    if (!running_flag_) throw BlockQueueStoppedException();
    queue_.emplace(item);
    cond_.notify_one();
  }
  // cqmark 不是完美转发，这样才是完美转发
    // 使用模板参数 U 进行完美转发
    // template<typename U>
    // void Enqueue(U&& item) {
    //     std::unique_lock<std::mutex> lck(mutex_);
    //     if (!running_flag_) throw BockQueueStoppedException();
    //     queue_.emplace(std::forward<U>(item)); // 完美转发
    //     cond_.notify_one();
    // }

  void Enqueue(T &&item) {
    std::unique_lock<std::mutex> lck(mutex_);
    if (!running_flag_) throw BlockQueueStoppedException();
    queue_.emplace(std::move(item));
    cond_.notify_one();
  }
  //cqmark wait函数调用的时候会释放锁，等待条件满足的时候再重新获得锁
  //如果没有就等待，等到有了再出列
  T Dequeue() {
    std::unique_lock<std::mutex> lck(mutex_);
    cond_.wait(lck, [this] { return !queue_.empty() || !running_flag_; });
    if (!running_flag_) throw BlockQueueStoppedException();
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }
  //cqmark 如果没有也不等待，直接返回nullptr
  std::optional<T> TryDequeue() {
    std::lock_guard<std::mutex> lck(mutex_);
    if (queue_.empty() || !running_flag_) [[unlikely]]
      return std::nullopt;

    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  void Stop() {
    std::unique_lock<std::mutex> lck(mutex_);
    running_flag_ = false;
    cond_.notify_all();
  }

  size_t Size() const {
    std::lock_guard<std::mutex> lck(mutex_);
    return queue_.size();
  }

  bool IsRunning() const {
    std::lock_guard<std::mutex> lck(mutex_);
    return running_flag_;
  }

 protected:
  mutable std::mutex mutex_;
  std::condition_variable cond_;
  std::queue<T> queue_;
  bool running_flag_ = true;
};
}  // namespace aimrt::common::util
