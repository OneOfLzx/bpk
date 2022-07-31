#ifndef THREADPOOL_THREAD_POOL_H
#define THREADPOOL_THREAD_POOL_H

#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <iostream>
using namespace std;

class ThreadPool
{
public:
    ThreadPool(size_t threads, bool isTaskFirst = false);
    ~ThreadPool();
    size_t getNumberOfThreads() const noexcept
    {
        return this->threads;
    }

    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                std::abort();

            taskQueue.emplace([task]() {
                (*task)();
            });
        }
        condition.notify_one();
        return res;
    }

private:
    size_t threads;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> taskQueue;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    bool isTaskFirst;
    unsigned runningThread;
    std::condition_variable stopCondition;
};

#endif
