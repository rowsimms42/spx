#pragma once

#include <concepts>
#include <mutex>
#include <optional>
#include <vector>

/**
 * Thread safe queue implementation.
 */
namespace spx {

  template <typename Lock>
  concept is_lockable = requires(Lock&& lock) {
    lock.lock();
    lock.unlock();
    requires(std::convertible_to<decltype(lock.try_lock()), bool>);
  };

  template <typename T, typename Lock = std::mutex>
  requires is_lockable<Lock>
  class SpxQueue {
  public:
    using value_type = T;
    using size_type = typename std::vector<T>::size_type;

    SpxQueue() = default;

    SpxQueue(SpxQueue&& other) noexcept {
      std::unique_lock lock(mutex_, std::defer_lock);
      std::unique_lock other_lock(other.mutex_, std::defer_lock);
      std::lock(lock, other_lock);
      data_ = std::move(other.data_);
    }

    SpxQueue& operator=(SpxQueue&& other) noexcept {
      if (this != &other) {
        std::unique_lock lock(mutex_, std::defer_lock);
        std::unique_lock other_lock(other.mutex_, std::defer_lock);
        std::lock(lock, other_lock);
        data_ = std::move(other.data_);
      }
      return *this;
    }

    /**
     * Adds an element to the queue.
     *
     * @param value the value to add
     */
    void push(T&& value) {
      std::lock_guard lock(mutex_);
      data_.emplace_back(std::move(value));
    }

    /**
     * Pops the next available element off the front of the queue.
     *
     * @return the element
     */
    std::optional<T> pop() {
      std::lock_guard lock(mutex_);
      if (data_.empty()) {
        return std::nullopt;
      }
      std::optional<T> front(std::move(data_.front()));
      data_.erase(data_.begin());
      return front;
    }

    /**
     * Steals an element from the back of the queue.
     *
     * @return the element
     */
    std::optional<T> steal() {
      std::lock_guard lock(mutex_);
      if (data_.empty()) {
        return std::nullopt;
      }
      std::optional<T> back(std::move(data_.back()));
      data_.pop_back();
      return back;
    }

    /**
     * Checks to see if the queue is empty.
     *
     * @return true if empty, false otherwise
     */
    bool empty() const {
      std::lock_guard lock(mutex_);
      bool is_empty = data_.empty();
      return is_empty;
    }

    /**
     * Convenience method to check the size of the queue.
     *
     * @return the size of the queue
     */
    size_type size() const {
      std::lock_guard lock(mutex_);
      size_type sz = data_.size();
      return sz;
    }

    /**
     * Convenience method to clear the queue.
     */
    void clear() {
      std::lock_guard lock(mutex_);
      data_.clear();
    }

  private:
    std::vector<T> data_{};
    mutable Lock mutex_{};
  };

} // spx
