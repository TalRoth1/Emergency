#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "../include/Frame.h"

class ThreadSafeQueue
{
private:
    std::queue<Frame> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void push(const Frame &data);

    Frame pop();

    bool empty();
};
