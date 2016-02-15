#include "ring_span.h"
#include <cassert>

#include <array>
#include <future>
#include <iostream>

using std::experimental::ring_span;

void ring_test()
{
    std::array<int,5> buffer;
    auto Q = ring_span<int>(buffer.begin(), buffer.end(), buffer.begin(), 0);

    Q.push_back(7);
    Q.push_back(3);
    assert(Q.size() == 2);
    assert(Q.front() == 7);

    Q.pop_front();
    assert(Q.size() == 1);

    Q.push_back(18);
    auto Q2 = Q;
    assert(Q2.front() == 3);
    assert(Q2.back() == 18);

    auto Q3 = std::move(Q2);
    assert(Q3.front() == 3);
    assert(Q3.back() == 18);

    auto Q4(Q3);
    assert(Q4.front() == 3);
    assert(Q4.back() == 18);

    auto Q5(std::move(Q3));
    assert(Q5.front() == 3);
    assert(Q5.back() == 18);
    assert(Q5.size() == 2);

    Q5.pop_front();
    Q5.pop_front();
    assert(Q5.empty());

    std::array<int,5> buffer2;
    auto Q6 = ring_span<int>(buffer2.begin(), buffer2.end(), buffer2.begin(), 0);
    Q6.push_back(6);
    Q6.push_back(7);
    Q6.push_back(8);
    Q6.push_back(9);
    Q6.emplace_back(10);  // TODO FIXME BUG HACK: P0059R1 has "emplace", not "emplace_back"
    Q6.swap(Q5);
    assert(Q6.empty());
    assert(Q5.size() == 5);
    assert(Q5.front() == 6);
    assert(Q5.back() == 10);
}

void thread_communication_test()
{
    std::array<int, 10> buffer;
    std::mutex m;
    std::condition_variable cv;
    auto r = ring_span<int>(buffer.begin(), buffer.end(), buffer.begin(), 0);

    auto ci = std::async(std::launch::async, [&]()
    {
        int val = 0;
        do
        {
            std::cin >> val;
            {
                std::lock_guard<std::mutex> lg(m);
                r.push_back(val);
                cv.notify_one();
            }
        } while (val != -1);
    });

    auto po = std::async(std::launch::async, [&]()
    {
        int val = 0;
        do
        {
            std::unique_lock<std::mutex> lk(m);
            while (r.empty()) cv.wait(lk);  // TODO FIXME BUG HACK: P0059R1 omits this loop
            val = r.pop_front();  // TODO FIXME BUG HACK: P0059R1 has "pop", not "pop_front"
            std::cout << val << std::endl;
            lk.unlock();  // TODO FIXME BUG HACK: this is unnecessary
        } while (val != -1);
    });
}
