//
// Created by 廖坤 on 2019/3/23.
//

#ifndef THREADPOOL_THREAD_H
#define THREADPOOL_THREAD_H

#include <functional>
#include <thread>

class Thread {
public:
    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc func)
            : func_(func) {}


    ~Thread() = default;

    std::thread::id start() {
        std::thread t(func_);
        t.detach();
        return t.get_id();
    }


private:
    ThreadFunc func_;
};


#endif //
