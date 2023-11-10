//
// Created by 廖坤 on 2019/3/23.
//

#ifndef THREADPOOL_THREAD_H
#define THREADPOOL_THREAD_H

#include <functional>
#include <thread>

class Thread {
public:
    using ThreadFunc = std::function<void(int)>;
    Thread(ThreadFunc func)
            : func_(func), threadId_(generateId_++) {}


    ~Thread() = default;

    void start() {
        std::thread t(func_, threadId_);
        t.detach();
    }

    int getId() const {
        return threadId_;
    }

private:
    ThreadFunc func_;
    static int generateId_;
    int threadId_;
};

int Thread::generateId_ = 0;
#endif //
