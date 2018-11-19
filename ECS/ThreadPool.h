#ifndef ThreadPool_H
#define ThreadPool_H

#include <algorithm>
#include <condition_variable>
#include <future>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <iostream>

namespace JT {
  class ThreadPool{
  public:
    ThreadPool() : stop(false) { this->MakeThread(8); }
    ThreadPool(size_t _s) :stop(false) { this->MakeThread(_s); }

    void enqueue(std::function<void()>);

    ~ThreadPool();

    void join();

  private:
    void MakeThread(size_t);
    // 일을 할 쓰레드들
    std::vector<std::thread> workers_;
    // 타스크
    std::queue<std::function<void()>> task_;
    
    std::condition_variable worker_condition;
    std::mutex task_mutex;

    bool stop;    
  };

  inline void ThreadPool::MakeThread(size_t _s) {
    for (size_t i = 0; i < _s; i++) {
      workers_.emplace_back([this] {
        for (;;)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->task_mutex);

            this->worker_condition.wait(lock,
              [this] { return this->stop || !this->task_.empty(); });


            if (this->stop && this->task_.empty()) {
              return;
            }
            task = std::move(this->task_.front());
            this->task_.pop();
          }
          task();
        }
      });
    }
  }

  void ThreadPool::enqueue(std::function<void()> _func) {
    {
      std::unique_lock<std::mutex> lock(task_mutex);
      task_.emplace(_func);
    }
    worker_condition.notify_one();
  }

  inline ThreadPool::~ThreadPool() {
    std::cout << "소멸자 실행" << std::endl;
    {
      std::unique_lock<std::mutex> lock(task_mutex);
      stop = true;
    }
    worker_condition.notify_all();
    for (std::thread &worker : workers_)
      worker.join();
  }
  inline void ThreadPool::join() {
    while (!task_.empty()){}
    return;
  }
}
#endif // !ThreadPool_H