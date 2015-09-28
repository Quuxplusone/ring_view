#include "fixed_ring.h"
#include <cassert>
#include <cstdio>

int main()
{
    fixed_ring<int, 4> fr;
    assert(fr.size() == 0);
    fr.push(1);
    assert(fr.size() == 1); assert(fr.front() == 1); assert(fr.back() == 1);
    fr.push(2);
    assert(fr.size() == 2); assert(fr.front() == 1); assert(fr.back() == 2);
    fr.push(3);
    assert(fr.size() == 3); assert(fr.front() == 1); assert(fr.back() == 3);
    fr.push(4);
    assert(fr.size() == 4); assert(fr.front() == 1); assert(fr.back() == 4);
    fr.push(5);
    assert(fr.size() == 4); assert(fr.front() == 2); assert(fr.back() == 5);
    fr.push(6);
    assert(fr.size() == 4); assert(fr.front() == 3); assert(fr.back() == 6);
    fr.pop();
    assert(fr.size() == 3); assert(fr.front() == 4); assert(fr.back() == 6);
}
