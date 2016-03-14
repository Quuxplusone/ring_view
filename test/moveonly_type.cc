
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <deque>
#include <string>
#include <vector>

#include "ring_span.h"


std::deque<int> expected_destructions;
struct S {
    int val;
    S(int i): val(i) {}
    S(const S&) = delete;
    S(S&&) = delete;
    S& operator=(const S&) = delete;
    S& operator=(S&&) = delete;
    ~S() {
        assert(val == expected_destructions.front());
        expected_destructions.pop_front();
    }
};

using ptr = std::unique_ptr<S>;

template<typename RingSpan>
std::string rv_to_string(const RingSpan& rv)
{
    std::string result;
    for (auto&& elt : rv) {
        assert(elt);
        result += std::to_string(elt->val);
        result += " ";
    }
    if (not rv.empty()) {
        result.resize(result.length() - 1);
    }
    return result;
}

template<typename RingSpan>
void assert_is(const RingSpan& rv, const std::string& expected)
{
    std::string rvs = rv_to_string(rv);
    if (rvs != expected) {
        fprintf(stderr, "Failed assert_is: %s != %s\n", rvs.c_str(), expected.c_str());
        exit(1);
    }
}

void test_null_popper()
{
    expected_destructions = { 2, 1, 4, 3, 7, 8, 10, 5, 9, 6 };
    std::array<ptr, 4> buffer;
    {
        std::experimental::ring_span<ptr, std::experimental::null_popper<ptr>> rv(buffer.begin(), buffer.end(), buffer.begin(), 0);
        rv.push_back(std::make_unique<S>(1));  assert_is(rv, "1");
        rv.push_back(std::make_unique<S>(2));  assert_is(rv, "1 2");
        rv.pop_back();                         assert_is(rv, "1");
        rv.pop_front();                        assert_is(rv, "");
        rv.push_back(std::make_unique<S>(3));  assert_is(rv, "3"); // (destroys 2)
        rv.push_front(std::make_unique<S>(4)); assert_is(rv, "4 3"); // (destroys 1)
        rv.push_back(std::make_unique<S>(5));  assert_is(rv, "4 3 5");
        rv.push_back(std::make_unique<S>(6));  assert_is(rv, "4 3 5 6");
        rv.push_back(std::make_unique<S>(7));  assert_is(rv, "3 5 6 7");
        rv.push_back(std::make_unique<S>(8));  assert_is(rv, "5 6 7 8");
        rv.pop_back();                         assert_is(rv, "5 6 7");
        rv.pop_back();                         assert_is(rv, "5 6");
        rv.push_back(std::make_unique<S>(9));  assert_is(rv, "5 6 9"); // (destroys 7)
        rv.push_back(std::make_unique<S>(10)); assert_is(rv, "5 6 9 10"); // (destroys 8)
    }

    // Ring test is done.
    // Destroy the rest of the buffer elements in a nice defined order.
    // The underlying buffer at this point is "9 10 5 6".
    buffer[1] = nullptr;  // 9 x 5 6
    buffer[2] = nullptr;  // 9 x x 6
    buffer[0] = nullptr;  // x x x 6
}

void test_move_popper()
{
    expected_destructions = { 2, 1, 4, 3, 8, 7, 10, 5, 9, 6 };
    std::array<ptr, 4> buffer;
    {
        std::experimental::ring_span<ptr> rv(buffer.begin(), buffer.end(), buffer.begin(), 0);
        rv.push_back(std::make_unique<S>(1));  assert_is(rv, "1");
        rv.push_back(std::make_unique<S>(2));  assert_is(rv, "1 2");
        rv.pop_back();                         assert_is(rv, "1");
        rv.pop_front();                        assert_is(rv, "");
        rv.push_back(std::make_unique<S>(3));  assert_is(rv, "3");
        rv.push_front(std::make_unique<S>(4)); assert_is(rv, "4 3");
        rv.push_back(std::make_unique<S>(5));  assert_is(rv, "4 3 5");
        rv.push_back(std::make_unique<S>(6));  assert_is(rv, "4 3 5 6");
        rv.push_back(std::make_unique<S>(7));  assert_is(rv, "3 5 6 7");
        rv.push_back(std::make_unique<S>(8));  assert_is(rv, "5 6 7 8");
        rv.pop_back();                         assert_is(rv, "5 6 7");
        rv.pop_back();                         assert_is(rv, "5 6");
        rv.push_back(std::make_unique<S>(9));  assert_is(rv, "5 6 9");
        rv.push_back(std::make_unique<S>(10));  assert_is(rv, "5 6 9 10");
    }

    // Ring test is done.
    // Destroy the rest of the buffer elements in a nice defined order.
    // The underlying buffer at this point is "9 10 5 6".
    buffer[1] = nullptr;  // 9 x 5 6
    buffer[2] = nullptr;  // 9 x x 6
    buffer[0] = nullptr;  // x x x 6
}

int main()
{
    test_null_popper();
    test_move_popper();
}
