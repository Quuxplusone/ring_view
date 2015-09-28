#pragma once

// On the subject of "API conventions for pushing into a fixed-size container",
// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4416.pdf
//

// Goal: A non-owning ring-buffer class.
// Since it's non-owning, it's not allowed to construct or destroy elements
// of the underlying container. Push and pop are implemented as assignment and
// bookkeeping, respectively.
//

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

// detail
template<class, bool> class ring_view_iterator;

template<class T>
struct default_popper {
    void operator()(T&) { }  // do nothing, return void
};

// Another useful popper class would be this one, which moves-from the
// popped element. This lets us make a ring-buffer of shared_ptr or
// unique_ptr with the appropriate semantics.
//
template<class T>
struct move_popper {
    T operator()(T& t) { return std::move(t); }
};

template<class T, class Popper = default_popper<T>>
class ring_view
{
public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using iterator = ring_view_iterator<ring_view, false>;
    using const_iterator = ring_view_iterator<ring_view, true>;

    // Construct a full ring_view.
    //
    template<class ContiguousIterator>
    ring_view(ContiguousIterator begin, ContiguousIterator end, Popper p = Popper()) noexcept :
        data_(&*begin),
        size_(end - begin),
        capacity_(end - begin),
        front_idx_(0),
        popper_(std::move(p))
    {}

    // Construct a "partially full" ring_view.
    //
    template<class ContiguousIterator>
    ring_view(ContiguousIterator begin, ContiguousIterator end, ContiguousIterator first, size_type size, Popper p = Popper()) noexcept :
        data_(&*begin),
        size_(size),
        capacity_(end - begin),
        front_idx_(first - begin),
        popper_(std::move(p))
    {}

    // Notice that an iterator contains a pointer to the ring_view itself.
    // Destroying ring_view rv invalidates rv.begin(), just as with an owning container.
    //
    iterator begin() noexcept { return iterator(0, this); }
    iterator end() noexcept { return iterator(size(), this); }
    const_iterator begin() const noexcept { return const_iterator(0, this); }
    const_iterator end() const noexcept { return const_iterator(size(), this); }

    reference front() noexcept { return *begin(); }
    reference back() noexcept { return *(end() - 1); }
    const_reference front() const noexcept { return *begin(); }
    const_reference back() const noexcept { return *(end() - 1); }

    bool empty() const noexcept { return size_ == 0; }
    bool full() const noexcept { return size_ == capacity_; }
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }

    // pop_front() increments the index of the begin of the ring.
    // Notice that it does not destroy anything.
    // Calling pop_front() on an empty ring is undefined,
    // in the same way as calling pop_front() on an empty vector or list is undefined.
    // Without pop_front(), you can't use ring_view as a std::queue.
    //
    auto pop_front()
    {
        assert(!empty());
        auto old_front_idx = front_idx_;
        front_idx_ = (front_idx_ + 1) % capacity_;
        --size_;
        return popper_(data_[old_front_idx]);
    }

    // push_back() assigns a new value to the element
    // at the end of the ring, and makes that element the
    // new back of the ring. If the ring is full before
    // the call to push_back(), we rotate the indices and
    // invalidate all iterators into the ring.
    // Without push_back(), you can't use ring_view as a std::queue.
    //
    template<bool b=true, typename=std::enable_if_t<b && std::is_copy_assignable<T>::value>>
    void push_back(const T& value) noexcept(std::is_nothrow_copy_assignable<T>::value)
    {
        data_[back_idx()] = value;
        if (not full()) {
            ++size_;
        }
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_move_assignable<T>::value>>
    auto push_back(T&& value) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        data_[back_idx()] = std::move(value);
        if (not full()) {
            ++size_;
        }
    }

private:
    friend class ring_view_iterator<ring_view, true>;
    friend class ring_view_iterator<ring_view, false>;

    // Should this member function be public?
    //
    reference at(size_type i) noexcept { return data_[(front_idx_ + i) % capacity_]; }
    const_reference at(size_type i) const noexcept { return data_[(front_idx_ + i) % capacity_]; }

    size_type back_idx() const noexcept { return (front_idx_ + size_) % capacity_; }

    T *data_;
    size_type size_;
    size_type capacity_;
    size_type front_idx_;
    Popper popper_;
};

template<class RV, bool is_const>
class ring_view_iterator
{
public:
    using value_type = typename RV::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::conditional_t<is_const, const value_type, value_type>*;
    using reference = typename std::conditional_t<is_const, const value_type, value_type>&;
    using iterator_category = std::random_access_iterator_tag;

    ring_view_iterator() = default;

    reference operator*() const noexcept { return rv_->at(idx_); }
    ring_view_iterator& operator++() noexcept { ++idx_; return *this; }
    ring_view_iterator operator++(int) noexcept { auto r(*this); ++*this; return r; }
    ring_view_iterator& operator--() noexcept { ++idx_; return *this; }
    ring_view_iterator operator--(int) noexcept { auto r(*this); ++*this; return r; }

    friend ring_view_iterator& operator+=(ring_view_iterator& it, int i) noexcept { it.idx_ += i; return it; }
    friend ring_view_iterator& operator-=(ring_view_iterator& it, int i) noexcept { it.idx_ -= i; return it; }
    friend ring_view_iterator operator+(ring_view_iterator it, int i) noexcept { it += i; return it; }
    friend ring_view_iterator operator-(ring_view_iterator it, int i) noexcept { it -= i; return it; }

    template<bool C> bool operator==(const ring_view_iterator<RV,C>& rhs) const noexcept { return idx_ == rhs.idx_; }
    template<bool C> bool operator!=(const ring_view_iterator<RV,C>& rhs) const noexcept { return idx_ != rhs.idx_; }
    template<bool C> bool operator<(const ring_view_iterator<RV,C>& rhs) const noexcept { return idx_ < rhs.idx_; }
    template<bool C> bool operator<=(const ring_view_iterator<RV,C>& rhs) const noexcept { return idx_ <= rhs.idx_; }
    template<bool C> bool operator>(const ring_view_iterator<RV,C>& rhs) const noexcept { return idx_ > rhs.idx_; }
    template<bool C> bool operator>=(const ring_view_iterator<RV,C>& rhs) const noexcept { return idx_ >= rhs.idx_; }

private:
    friend RV;

    using size_type = typename RV::size_type;

    ring_view_iterator(size_type idx, RV *rv) noexcept : idx_(idx), rv_(rv) {}

    size_type idx_;
    RV *rv_;
};
