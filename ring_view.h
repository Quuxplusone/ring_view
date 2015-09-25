#pragma once

// On the subject of "API conventions for pushing into a fixed-size container",
// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4416.pdf
//

// Goal: A non-owning ring-buffer class.
// It should be empty-able, i.e., it should construct and destroy its elements.
// Since no objects are ever moved, it should support non-copy/moveable objects.
//
// However, for types that DO support assignment, it should be efficient to "overwrite";
// the operation of pop-and-then-push(v) should be POSSIBLE to achieve as a
// single assignment, if the user explicitly requests it.
//

#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>

// detail
template<class, bool> class ring_view_iterator;

template<class T>
class ring_view
{
public:
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using iterator = ring_view_iterator<T, false>;
    using const_iterator = ring_view_iterator<T, true>;

    // Construct an empty ring_view.
    //
    ring_view(void *data, size_type capacity) noexcept :
        data_(static_cast<T*>(data)),
        empty_(true),
        capacity_(capacity),
        front_idx_(0),
        back_idx_(0)
    {}

    // Construct a full ring_view.
    //
    template<class ContiguousIterator>
    ring_view(ContiguousIterator begin, ContiguousIterator end) noexcept :
        data_(&*begin),
        empty_(false),
        capacity_(end - begin),
        front_idx_(0),
        back_idx_(0)
    {}

    // Notice that an iterator contains a pointer to the ring_view
    // itself. Destroying ring_view rv invalidates rv.begin(),
    // just as with an owning container.
    //
    iterator begin() noexcept { return iterator(0, this); }
    iterator end() noexcept { return iterator(size(), this); }
    const_iterator begin() const noexcept { return const_iterator(0, this); }
    const_iterator end() const noexcept { return const_iterator(size(), this); }

    reference front() noexcept { return *begin(); }
    reference back() noexcept { return *(end() - 1); }
    const_reference front() const noexcept { return *begin(); }
    const_reference back() const noexcept { return *(end() - 1); }

    bool empty() const noexcept { return empty_; }
    bool full() const noexcept { return (not empty_) && (front_idx_ == back_idx_); }
    size_type size() const noexcept { return full() ? capacity_ : (back_idx_ - front_idx_ + capacity_) % capacity_; }
    size_type capacity() const noexcept { return capacity_; }

    // try_emplace_back() constructs a new element at the back of the ring.
    // If the ring is full, we return false and don't do anything else.
    //
    template<class... Args>
    bool try_emplace_back(Args&&... args)
    {
        if (full()) {
            return false;
        }
        emplace_back(std::forward<Args>(args)...);
        return true;
    }

    // Calling emplace_back() on a full ring is undefined,
    // in the same way as calling pop_front() on any empty container is undefined.
    // Without emplace_back(), you can't use ring_view as a std::queue.
    //
    template<class... Args>
    void emplace_back(Args&&... args)
    {
        assert(!full());
        new ((void*)&data_[back_idx_]) T(std::forward<Args>(args)...);
        back_idx_ = (back_idx_ + 1) % capacity_;
        empty_ = false;
    }

    // Calling push_back() on a full ring is undefined,
    // in the same way as calling pop_front() on any empty container is undefined.
    // Without push_back(), you can't use ring_view as a std::queue.
    //
    void push_back(const T& value) { emplace_back(value); }
    void push_back(T&& value) { emplace_back(std::move(value)); }

    // pop_front() destroys the element at the front of the ring.
    // Calling pop_front() on an empty ring is undefined,
    // in the same way as calling pop_front() on an empty vector or list is undefined.
    // Without pop_front(), you can't use ring_view as a std::queue.
    //
    bool pop_front()
    {
        assert(!empty());
        data_[front_idx_].~T();
        front_idx_ = (front_idx_ + 1) % capacity_;
        if (front_idx_ == back_idx_) {
            empty_ = true;
        }
        return true;
    }

    // assign_when_full() assigns a new value to the element
    // at the front of the ring, and makes that element the
    // new back of the ring by rotating the indices.
    // It invalidates all iterators into the ring.
    // If the ring is NOT full, undefined behavior.
    //
    void assign_when_full(const T& value)  // and another version for T&&
    {
        assert(full());
        data_[front_idx_] = value;
        back_idx_ = front_idx_ = (front_idx_ + 1) % capacity_;
    }

    // Should this be named push_back?
    // If the ring is full, this function invalidates all iterators into the ring.
    //
    template<class... Args>
    void emplace_or_assign(Args&&... args)
    {
        if (not try_emplace_back(std::forward<Args>(args)...)) {
            assign_when_full(T(std::forward<Args>(args)...));
        }
    }

private:
    friend class ring_view_iterator<T, true>;
    friend class ring_view_iterator<T, false>;

    // Should this member function be public?
    //
    reference at(size_type i) noexcept { return data_[(front_idx_ + i) % capacity_]; }
    const_reference at(size_type i) const noexcept { return data_[(front_idx_ + i) % capacity_]; }

    T *data_;
    bool empty_;
    size_type capacity_;
    size_type front_idx_;
    size_type back_idx_;
};

template<class T, bool is_const>
class ring_view_iterator
{
public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::conditional<is_const, const T, T>::type*;
    using reference = typename std::conditional<is_const, const T, T>::type&;
    using iterator_category = std::random_access_iterator_tag;

    ring_view_iterator() = default;

    reference operator*() const noexcept { return rv_->at(idx_); }
    ring_view_iterator& operator++() noexcept { ++idx_; return *this; }
    ring_view_iterator operator++(int) noexcept { auto r(*this); ++*this; return r; }
    ring_view_iterator& operator--() noexcept { ++idx_; return *this; }
    ring_view_iterator operator--(int) noexcept { auto r(*this); ++*this; return r; }

    template<bool C> bool operator==(const ring_view_iterator<T,C>& rhs) const noexcept { return idx_ == rhs.idx_; }
    template<bool C> bool operator!=(const ring_view_iterator<T,C>& rhs) const noexcept { return idx_ != rhs.idx_; }
    template<bool C> bool operator<(const ring_view_iterator<T,C>& rhs) const noexcept { return idx_ < rhs.idx_; }
    template<bool C> bool operator<=(const ring_view_iterator<T,C>& rhs) const noexcept { return idx_ <= rhs.idx_; }
    template<bool C> bool operator>(const ring_view_iterator<T,C>& rhs) const noexcept { return idx_ > rhs.idx_; }
    template<bool C> bool operator>=(const ring_view_iterator<T,C>& rhs) const noexcept { return idx_ >= rhs.idx_; }

private:
    friend class ring_view<T>;
    using size_type = typename ring_view<T>::size_type;

    ring_view_iterator(size_type idx, ring_view<T> *rv) noexcept : idx_(idx), rv_(rv) {}

    size_type idx_;
    ring_view<T> *rv_;
};

