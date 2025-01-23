#include "../include/ThreadSafeQueue.h"

void ThreadSafeQueue::push(const Frame &data)
{
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(data);
    cv.notify_one();
}

Frame ThreadSafeQueue::pop()
{
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !queue.empty(); });
    Frame output = queue.front();
    queue.pop();
    return output;
}

bool ThreadSafeQueue::empty()
{
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
}