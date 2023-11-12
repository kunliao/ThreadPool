// 线程池项目-最终版.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <functional>


using namespace std;


#include "ThreadPool.hpp"

int main() {
    ThreadPool *pool = new ThreadPool(0, 5);
    pool->start();


    for (int i = 0; i < 100; i++) {
        auto f = pool->submitTask([=]() -> int {
            this_thread::sleep_for(chrono::milliseconds(rand() % 1000));
            cout << "task " << i << endl;
            return rand();
        });

        int r = f.get();
    }
    //delete pool;
    //pool->shutdown();
    //pool->stop();
    getchar();

    int a = 90;
}