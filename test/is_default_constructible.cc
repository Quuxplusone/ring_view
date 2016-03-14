#include <memory>
#include <vector>

#include "ring_span.h"

int main()
{
    using std::experimental::ring_span;

    ring_span<int> rv1;
    ring_span<std::unique_ptr<int>> rv2;

    std::vector<ring_span<int>> vec;
}
