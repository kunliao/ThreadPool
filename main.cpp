// 线程池项目-最终版.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <functional>


#include "ThreadPool.hpp"

int main() {
    ThreadPool *pool = new ThreadPool(0, 5);
    pool->start();


    for (int i = 0; i < 100; i++) {
        std::future<int> future = pool->submitTask([=]() -> int {
            int r = rand();
            std::this_thread::sleep_for(std::chrono::milliseconds(r % 1000));
            std::cout << "task " << i << " r=" << r << std::endl;
            return r;
        });

        int ret = future.get();
        std::cout << "ret=" << ret << std::endl;
    }
    //delete pool;
    //pool->shutdown();
    //pool->stop();
    getchar();
    return 0;
}