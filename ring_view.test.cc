#include "ring.h"

#include <memory>
#include <queue>
#include <vector>

using namespace std::experimental;

using T = std::unique_ptr<int>;
using RVT = ring_span<T, move_popper<T>>;

int main()
{
    std::vector<T> vec(100);
    RVT rv(vec.begin(), vec.end());
    assert(rv.capacity() == 100);
    assert(rv.size() == 100);
    rv.push_back(nullptr);
    assert(rv.capacity() == 100);
    assert(rv.size() == 100);
    rv.pop_front();
    assert(rv.capacity() == 100);
    assert(rv.size() == 99);

    std::queue<T, RVT> q(rv);
    q.push(nullptr);
    q.push(nullptr);
    q.front();
    q.back();
    q.pop();
}
