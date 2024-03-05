#pragma once

#include <deque>
#include <condition_variable>
#include <iostream>

#define VERSION "0.1.1"

#define PROGRESS_BAR_NAME "Generating"

class PCM_QUEUE {
  std::deque<int16_t> queue;
  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<bool> done = false;
public:
  void push(int16_t* data, int size) {
    std::unique_lock<std::mutex> lock(mutex);
    for (int i = 0; i < size; i++) {
      queue.push_back(data[i]);
    }
    lock.unlock();
    cv.notify_one();
  }

  int pop(int16_t* data, int size) {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return done || queue.size() >= size; });
    int i = 0;
    while (i < size && !queue.empty()) {
      data[i] = queue.front();
      queue.pop_front();
      i++;
    }
    lock.unlock();
    cv.notify_one();
    return i;
  }

  size_t size() {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
  }

  void set_done() {
    std::unique_lock<std::mutex> lock(mutex);
    done = true;
    lock.unlock();
    cv.notify_one();
  }

  bool is_done() {
    return done;
  }
};