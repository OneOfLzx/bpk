#include "threadpool.h"
#include <stdexcept>

ThreadPool::ThreadPool(size_t threads, bool isTaskFirst)
    : threads(threads)
    , stop(false)
    , workers()
    , taskQueue()
    , queue_mutex()
    , condition()
    , isTaskFirst(isTaskFirst)
    , runningThread(0)
    , stopCondition()
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back([this]{
            for (;;)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->taskQueue.empty();
                    });
                    if (this->stop && this->taskQueue.empty())
                        return;
                    task = std::move(this->taskQueue.front());
                    this->taskQueue.pop();
                    this->runningThread++;
                }

                task();

                if (true == this->isTaskFirst)
                {
                    this->queue_mutex.lock();
                    this->runningThread--;
                    if (0 == this->runningThread && 0 == this->taskQueue.size())
                    {
                        this->stopCondition.notify_all();
                    }
                    this->queue_mutex.unlock();
                }
            }
        });
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (true == isTaskFirst)
        {
            if (!(0 == runningThread && 0 == taskQueue.size()))
            {
                stopCondition.wait(lock, [this] {
                    return (0 == this->runningThread && 0 == this->taskQueue.size());
                });
            }
        }

        stop = true;
    }

    condition.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}