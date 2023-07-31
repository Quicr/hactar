/*
 * Copyright (c) 2023 Cisco Systems, Inc. and others.  All rights reserved.
 */
#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <atomic>
#include <unistd.h>
#include <condition_variable>

namespace qtransport {

/**
 * @brief safeQueue is a thread safe basic queue
 *
 * @details This class is a thread safe wrapper for std::queue<T>.
 * 		Not all operators or methods are implemented.
 *
 * @todo Implement any operators or methods needed
 */
template<typename T>
class safeQueue
{
public:
  /**
   * @brief safeQueue constructor
   *
   * @param limit     Limit number of messages in queue before push blocks. Zero
   *                  is unlimited.
   */
  safeQueue(uint32_t limit = 1000)
    : stop_waiting{ false }
    , limit{ limit }
  {
  }

  ~safeQueue() { stopWaiting(); }

  /**
   * @brief inserts element at the end of queue
   *
   * @details Inserts element at the end of queue. If queue is at max size,
   *    the front element will be popped/removed to make room.
   *    In this sense, the queue is sliding forward with every new message
   *    added to queue.
   *
   * @param elem
   * @return True if successfully pushed, false if not.  The cause for false is
   * that the queue is full.
   */
  bool push(T const& elem)
  {
    bool rval = true;
    std::lock_guard<std::mutex> lock(mutex);

    if (queue.size() >= limit) {// Make room by removing first element
      queue.pop();
      rval = false;
    }

    queue.push(elem);

    if (limit && queue.size() >= limit) {
      is_full = true;
    }

    cv.notify_one();

    return rval;
  }

  /**
   * @brief Remove the first object from queue (oldest object)
   *
   * @return std::nullopt if queue is empty, otherwise reference to object
   */
  std::optional<T> pop()
  {
    std::lock_guard<std::mutex> lock(mutex);

    return pop_internal();
  }

  /**
    * @brief Get first object without removing from queue
    *
    * @return std::nullopt if queue is empty, otherwise reference to object
    */
  std::optional<T> front()
  {
    std::lock_guard<std::mutex> lock(mutex);

    if (queue.empty()) {
      return std::nullopt;
    }

    return queue.front();
  }

  /**
  * @brief Remove (aka pop) the first object from queue
  *
  */
  void pop_front()
  {
    std::lock_guard<std::mutex> lock(mutex);

    pop_front_internal();
  }



  /**
   * @brief Block waiting for data in queue, then remove the first object from
   * queue (oldest object)
   *
   * @details This will block if the queue is empty. Due to concurrency, it's
   * possible that when unblocked the queue might still be empty. In this case,
   * try again.
   *
   * @return std::nullopt if queue is empty, otherwise reference to object
   */
  std::optional<T> block_pop()
  {
    std::unique_lock<std::mutex> lock(mutex);

    cv.wait(lock, [&]() { return (stop_waiting || (queue.size() > 0)); });

    return pop_internal();
  }

  /**
   * @brief Size of the queue
   *
   * @return size of the queue
   */
  size_t size()
  {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
  }

  /**
   * @brief Put the queue in a state such that threads will not wait
   *
   * @return Nothing
   */
  void stopWaiting()
  {
    std::lock_guard<std::mutex> lock(mutex);
    cv.notify_all();
    stop_waiting = true;
  }

  void setLimit(uint32_t limit)
  {
    std::lock_guard<std::mutex> lock(mutex);
    this->limit = limit;
  }

private:

  /**
   * @brief Remove the first object from queue (oldest object)
   *
   * @return std::nullopt if queue is empty, otherwise reference to object
   *
   * @details The mutex must be locked by the caller
   */
  std::optional<T> pop_internal()
  {
    if (queue.empty()) {
      return std::nullopt;
    }

    auto elem = queue.front();
    queue.pop();

    if (queue.size() < limit)
      is_full = false;

    if (queue.size() > 0) {
      cv.notify_one();
    }

    return elem;
  }

  /**
 * @brief Remove the first object from queue (oldest object)
 *
 * @details The mutex must be locked by the caller
 */
  void pop_front_internal()
  {
    if (queue.empty()) {
      return;
    }

    if (queue.size() < limit)
      is_full = false;

    queue.pop();

    if (queue.size() > 0) {
      cv.notify_one();
    }
  }


  std::atomic<bool> is_full;    // Indicates if queue is full
  bool stop_waiting;            // Instruct threads to stop waiting
  uint32_t limit;               // Limit of number of messages in queue
  std::condition_variable cv;   // Signaling for thread syncronization
  std::mutex mutex;             // read/write lock
  std::queue<T> queue;          // Queue
};

} /* namespace qtransport */
