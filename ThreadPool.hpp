#pragma once

#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
#include "Thread.hpp"

const int TASK_CORE_THREAD_SIZE = 0;
const int TASK_MAX_THREAD_SIZE = 100;
const int TASKQUEUE_MAXSIZE = 10000;
const int THREAD_KEEP_ALIVE_TIME_ = 10;


class ThreadPool {
public:
    enum class State {
        INITIATED,
        RUNNING,
        STOPPING,
        STOP
    };

public:
    // 线程池构造
    ThreadPool(int coreThreadSize = TASK_CORE_THREAD_SIZE, int maxThreadSize = TASK_MAX_THREAD_SIZE,
               int maxTaskQueueSize = TASKQUEUE_MAXSIZE, int keepAliveTime = THREAD_KEEP_ALIVE_TIME_)
            : coreThreadSize_(coreThreadSize),
              idleThreadSize_(0),
              curThreadSize_(0),
              maxTaskQueueSize_(maxTaskQueueSize),
              maxThreadSize_(maxThreadSize),
              state_(State::INITIATED),
              keepAliveTime_(keepAliveTime) {
    }


    ~ThreadPool() {
        stop();
    }


    template<typename Func, typename... Args>
    auto submitTask(Func &&func, Args &&... args) -> std::future<decltype(func(args...))> {
        // 打包任务，放入任务队列里面
        using ReturnType = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<ReturnType> result = task->get_future();

        if (state_ != State::RUNNING) {
            throw "threadpool is not running!";
        }

        std::unique_lock<std::mutex> lock(mutex_);

        if (state_ == State::RUNNING && curThreadSize_ < maxThreadSize_ && idleThreadSize_ == 0) {
            auto t = std::unique_ptr<Thread>(
                    new Thread(std::bind(&ThreadPool::threadFunc, this)));
            std::thread::id threadId = t->start();
            threads_.emplace(threadId, std::move(t));
            curThreadSize_++;
        }

        // 用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败，返回
        if (!notFull_.wait_for(lock, std::chrono::seconds(1),
                               [&]() -> bool { return taskQue_.size() < (size_t) maxTaskQueueSize_; })) {

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                    []() -> ReturnType { return ReturnType(); });
            (*task)();
            return task->get_future();
        }
        taskQue_.emplace([task]() { (*task)(); });

        notEmpty_.notify_all();

        // 返回任务的Result对象
        return result;
    }

    // 开启线程池
    void start() {
        // 设置线程池的运行状态
        if (state_ == State::INITIATED)
            state_ = State::RUNNING;
    }

    void shutdown() {
        state_ = State::STOPPING;
        std::unique_lock<std::mutex> lock(mutex_);
        exitCond_.wait(lock, [&]() -> bool { return threads_.size() == 0; });
        state_ = State::STOP;
        curThreadSize_ = 0;
        curThreadSize_ = 0;
    }

    void stop() {
        if (state_ != State::STOP) {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!taskQue_.empty()) taskQue_.pop();
            threads_.clear();
        }
        state_ = State::STOP;
    }

    State getState() const {
        return state_;
    }

    bool isRunning() const {
        return state_ == State::RUNNING;
    }

    size_t getThreads() const {
        return curThreadSize_;
    }

    size_t getTaskQueueSize() const {
        return taskQue_.size();
    }

    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;


private:
    void threadFunc() {
        idleThreadSize_++;
        while (state_ == State::RUNNING || state_ == State::STOPPING) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);

                while (taskQue_.size() == 0) {
                    if (state_ == State::STOPPING) {
                        // 线程池要结束，回收线程资源
                        threads_.erase(std::this_thread::get_id()); //
                        exitCond_.notify_all();

                        goto _quit; // 线程函数结束，线程结束
                    }

                    //任务队列空闲后，线程等待keepAliveTime。然后如果当前线程数量比核心线程数量多，需要释放一部分线程
                    if (notEmpty_.wait_for(lock, std::chrono::seconds(keepAliveTime_)) ==
                        std::cv_status::timeout) {
                        if (curThreadSize_ > coreThreadSize_ && taskQue_.size() == 0) {
                            threads_.erase(std::this_thread::get_id()); //
                            exitCond_.notify_all();

                            goto _quit;
                        }
                    }
                }


                task = taskQue_.front();
                taskQue_.pop();

                if (taskQue_.size() > 0) {
                    notEmpty_.notify_all();
                }

                notFull_.notify_all();
            }

            // 当前线程负责执行这个任务
            if (task) {
                idleThreadSize_--;
                task();
                idleThreadSize_++;
            }
        }
        _quit:
        curThreadSize_--;
        idleThreadSize_--;
    }


private:
    std::unordered_map<std::thread::id, std::unique_ptr<Thread>> threads_; // 线程列表
    size_t coreThreadSize_;  // 初始的线程数量
    size_t maxThreadSize_; // 线程数量上限阈值
    std::atomic_int curThreadSize_;    // 记录当前线程池里面线程的总数量
    std::atomic_int idleThreadSize_; // 记录空闲线程的数量
    std::queue<std::function<void()>> taskQue_; // 任务队列
    size_t maxTaskQueueSize_;  // 任务队列数量上限阈值
    std::condition_variable notFull_; // 表示任务队列不满
    std::condition_variable notEmpty_; // 表示任务队列不空
    std::condition_variable exitCond_; // 等到线程资源全部回收
    std::atomic<State> state_;
    std::mutex mutex_;
    int keepAliveTime_;
};


