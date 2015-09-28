
#include "ring_view.h"

#include <array>
#include <cassert>
#include <memory>

// An example of how to use the ring_view as a "history".
void add_to_history(ring_view<int> rv, int new_value)
{
    if (not rv.try_emplace_back(new_value)) {
        rv.assign_when_full(new_value);
    }
}

// An example of how to use the ring_view as a concurrent_bounded_queue. Ish.
template<class T>
struct unique_bounded_queue {
    using raw_T = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    unique_bounded_queue(size_t cap) :
        data_(std::get_temporary_buffer<T>(cap).first, get_deleter()),  // no error-checking, mind you
        rv_(data_.get(), cap)
    {
        assert(rv_.capacity() == cap);
        assert(rv_.size() == 0);
    }

    void push(const T& value) { rv_.emplace_or_assign(value); }
    void push(T&& value) { rv_.emplace_or_assign(value); }
    void pop() { rv_.pop_front(); }
    auto begin() { return rv_.begin(); }
    auto end() { return rv_.end(); }

private:
    static auto get_deleter() { return [](T *p){ std::return_temporary_buffer(p); }; }

    std::unique_ptr<T, decltype(get_deleter())> data_;
    ring_view<T> rv_;
};

int main()
{
    unique_bounded_queue<int> ubq(4);
    for (int v : {1,2,3,4,5,6,7,8}) {
        ubq.push(v);
        for (int i : ubq) {
            printf(" %d", i);
        }
        printf("\n");
    }

    // Test "observing" generic bytes as if they were `T`...
    {
        // Keep track of ctor and dtor calls.
        int ctor_count = 0;
        int dtor_count = 0;

        // Example item that increments the above variables on ctor/dtor.
        struct example_item
        {
            int& ctor_count_ref;
            int& dtor_count_ref;

            example_item(int& ccr, int& dcr) : ctor_count_ref(ccr), dtor_count_ref(dcr) { ++ctor_count_ref; }
            ~example_item() { ++dtor_count_ref; }
        };

        // Create some bytes.
        using example_raw_storage = std::aligned_storage_t<sizeof(example_item), alignof(example_item)>;
        using example_item_raw_array = std::array<example_raw_storage, 32>;
        example_item_raw_array eira;

        // Look at those bytes as if they were `example_item` instances.
        ring_view<example_item> rw(eira.data(), 8);

        assert(rw.empty());
        assert(rw.size() == 0);
        assert(rw.capacity() == 8);

        assert(ctor_count == 0);
        assert(dtor_count == 0);

        rw.emplace_back(ctor_count, dtor_count);
        rw.emplace_back(ctor_count, dtor_count);
        rw.emplace_back(ctor_count, dtor_count);

        assert(!rw.empty());
        assert(rw.size() == 3);
        assert(rw.capacity() == 8);

        assert(ctor_count == 3);
        assert(dtor_count == 0);

        rw.pop_front();
        rw.pop_front();
        rw.pop_front();

        assert(rw.empty());
        assert(rw.size() == 0);
        assert(rw.capacity() == 8);

        assert(ctor_count == 3);
        assert(dtor_count == 3);
    }
}
