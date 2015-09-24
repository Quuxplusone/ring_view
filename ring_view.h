#pragma once

// On the subject of "API conventions for pushing into a fixed-size container",
// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4416.pdf
//

template<class T>
class ring_view {
    typedef T* pointer;
    typedef T& reference;
    typedef size_t size_type;
    typedef ... iterator;
    typedef ... const_iterator;

    ring_view(T *data, size_type capacity) :
        data_(data),
        size_(0),
        capacity_(capacity),
        front_idx_(0),
        back_idx_(0)
    {}

    template<class ContiguousIterator>
    ring_view(ContiguousIterator begin, ContiguousIterator end) :
        data_(&*begin),
        size_(0),
        capacity_(&*end - &*begin),
        front_idx_(0),
        back_idx_(0)
    {}

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    reference front() { return *begin(); }
    reference back() { return *(end() - 1); }
    const_reference front() const { return *begin(); }
    const_reference back() const { return *(end() - 1); }

    bool empty() const { return empty_; }
    bool full() const { return (not empty_) && (front_idx_ == back_idx_); }
    size_type size() const { return full() ? capacity_ : (back_idx_ - front_idx_ + capacity_) % capacity_; }
    size_type capacity() const { return capacity_; }

    // try_emplace_back() constructs a new element at the back of the ring.
    // If the ring is full, we return false and don't do anything else.
    //
    template<class... Args>
    bool try_emplace_back(Args&&... args)
    {
        if (full()) {
            return false;
        }
        new ((void*)&data_[back_idx_]) T(std::forward<Args>(args)...);
        back_idx_ = (back_idx_ + 1) % capacity_;
        empty_ = false;
        return true;
    }

    // pop_front() destroys the element at the front of the ring.
    // If the ring is empty, we return false and don't do anything else.
    //
    bool pop_front()
    {
        if (empty()) {
            return false;
        }
        data_[front_idx_]->~T();
        front_idx_ = (front_idx_ + 1) % capacity_;
        if (front_idx_ == back_idx_) {
            empty_ = true;
        }
        return true;
    }

    // assign_when_full() assigns a new value to the element
    // at the front of the ring, and makes that element the
    // new back of the ring by rotating the indices.
    // If the ring is NOT full, undefined behavior.
    //
    void assign_when_full(const T& value)  // and another version for T&&
    {
        ASSERT(full());
        data_[front_idx_] = value;
        back_idx_ = front_idx_ = (front_idx_ + 1) % capacity_;
    }

    void emplace_or_assign(Args&&... args)
    {
        if (not try_emplace(std::forward<Args>(args)...)) {
            assign_when_full(T(std::forward<Args>(args)...));
        }
    }

private:
    T *data_;
    bool empty_;
    size_type capacity_;
    size_type front_idx_;
    size_type back_idx_;
};


#ifdef TESTING

// An example of how to use the ring_view as a "history".
void add_to_history(ring_view<int> rv, int new_value)
{
    if (not rv.try_emplace_back(new_value)) {
        rv.assign_when_full(new_value);
    }
}

#endif /* TESTING */
