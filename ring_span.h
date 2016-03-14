#pragma once

// Reference implementation of P0059R1 + errata.

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace std { namespace experimental {

namespace detail {
    template<class, bool> class ring_iterator;
} // namespace detail

template<class T>
struct null_popper {
    void operator()(T&) { }
};

template<class T>
struct move_popper {
    T operator()(T& t) { return std::move(t); }
};

template<class T, class Popper = move_popper<T>>
class ring_span
{
public:
    using type = ring_span<T, Popper>;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using iterator = detail::ring_iterator<ring_span, false>;  // exposition only
    using const_iterator = detail::ring_iterator<ring_span, true>;  // exposition only
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    template<class ContiguousIterator>
    ring_span(ContiguousIterator begin, ContiguousIterator end, Popper p = Popper()) noexcept :
        data_(&*begin),
        size_(end - begin),
        capacity_(end - begin),
        front_idx_(0),
        popper_(std::move(p))
    {}

    template<class ContiguousIterator>
    ring_span(ContiguousIterator begin, ContiguousIterator end, ContiguousIterator first, size_type size, Popper p = Popper()) noexcept :
        data_(&*begin),
        size_(size),
        capacity_(end - begin),
        front_idx_(first - begin),
        popper_(std::move(p))
    {}

    iterator begin() noexcept { return iterator(0, this); }
    iterator end() noexcept { return iterator(size(), this); }
    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }
    const_iterator cbegin() const noexcept { return const_iterator(0, this); }
    const_iterator cend() const noexcept { return const_iterator(size(), this); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    const_reverse_iterator rend() const noexcept { return crend(); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    reference front() noexcept { return *begin(); }
    reference back() noexcept { return *(end() - 1); }
    const_reference front() const noexcept { return *begin(); }
    const_reference back() const noexcept { return *(end() - 1); }

    bool empty() const noexcept { return size_ == 0; }
    bool full() const noexcept { return size_ == capacity_; }
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }

    auto pop_front()
    {
        assert(not empty());
        auto& elt = front_();
        increment_front_();
        return popper_(elt);
    }

    auto pop_back()
    {
        assert(not empty());
        auto& elt = back_();
        decrement_back_();
        return popper_(elt);
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_copy_assignable<T>::value>>
    void push_back(const T& value) noexcept(std::is_nothrow_copy_assignable<T>::value)
    {
        if (full()) {
            increment_front_and_back_();
        } else {
            increment_back_();
        }
        back_() = value;
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_move_assignable<T>::value>>
    void push_back(T&& value) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        if (full()) {
            increment_front_and_back_();
        } else {
            increment_back_();
        }
        back_() = std::move(value);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value && std::is_nothrow_move_assignable<T>::value)
    {
        if (full()) {
            increment_front_and_back_();
        } else {
            increment_back_();
        }
        back_() = T(std::forward<Args>(args)...);
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_copy_assignable<T>::value>>
    void push_front(const T& value) noexcept(std::is_nothrow_copy_assignable<T>::value)
    {
        if (full()) {
            decrement_front_and_back_();
        } else {
            decrement_front_();
        }
        front_() = value;
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_move_assignable<T>::value>>
    void push_front(T&& value) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        if (full()) {
            decrement_front_and_back_();
        } else {
            decrement_front_();
        }
        front_() = std::move(value);
    }

    template<typename... Args>
    void emplace_front(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value && std::is_nothrow_move_assignable<T>::value)
    {
        if (full()) {
            decrement_front_and_back_();
        } else {
            decrement_front_();
        }
        front_() = T(std::forward<Args>(args)...);
    }

    void swap(ring_span& rhs) noexcept(std::__is_nothrow_swappable<Popper>::value)
    {
        using std::swap;
        swap(data_, rhs.data_);
        swap(size_, rhs.size_);
        swap(capacity_, rhs.capacity_);
        swap(front_idx_, rhs.front_idx_);
        swap(popper_, rhs.popper_);
    }

    friend void swap(ring_span& lhs, ring_span& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

private:
    // all private members are exposition-only

    friend class detail::ring_iterator<ring_span, true>;
    friend class detail::ring_iterator<ring_span, false>;

    reference at(size_type i) noexcept { return data_[(front_idx_ + i) % capacity_]; }
    const_reference at(size_type i) const noexcept { return data_[(front_idx_ + i) % capacity_]; }

    reference front_() noexcept { return *(data_ + front_idx_); }
    const_reference front_() const noexcept { return *(data_ + front_idx_); }
    reference back_() noexcept { return *(data_ + (front_idx_ + size_ - 1) % capacity_); }
    const_reference back_() const noexcept { return *(data_ + (front_idx_ + size_ - 1) % capacity_); }

    void increment_front_() noexcept {
        front_idx_ = (front_idx_ + 1) % capacity_;
        --size_;
    }
    void decrement_front_() noexcept {
        front_idx_ = (front_idx_ + capacity_ - 1) % capacity_;
        ++size_;
    }

    void increment_back_() noexcept {
        ++size_;
    }
    void decrement_back_() noexcept {
        --size_;
    }

    void increment_front_and_back_() noexcept {
        front_idx_ = (front_idx_ + 1) % capacity_;
    }
    void decrement_front_and_back_() noexcept {
        front_idx_ = (front_idx_ + capacity_ - 1) % capacity_;
    }

    T *data_;
    size_type size_;
    size_type capacity_;
    size_type front_idx_;
    Popper popper_;
};

namespace detail {

template<class RV, bool is_const>
class ring_iterator
{
public:
    using type = ring_iterator<RV, is_const>;
    using value_type = typename RV::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::conditional_t<is_const, const value_type, value_type>*;
    using reference = typename std::conditional_t<is_const, const value_type, value_type>&;
    using iterator_category = std::random_access_iterator_tag;

    ring_iterator() = default;

    reference operator*() const noexcept { return rv_->at(idx_); }
    ring_iterator& operator++() noexcept { ++idx_; return *this; }
    ring_iterator operator++(int) noexcept { auto r(*this); ++*this; return r; }
    ring_iterator& operator--() noexcept { ++idx_; return *this; }
    ring_iterator operator--(int) noexcept { auto r(*this); ++*this; return r; }

    friend ring_iterator& operator+=(ring_iterator& it, int i) noexcept { it.idx_ += i; return it; }
    friend ring_iterator& operator-=(ring_iterator& it, int i) noexcept { it.idx_ -= i; return it; }
    friend ring_iterator operator+(ring_iterator it, int i) noexcept { it += i; return it; }
    friend ring_iterator operator-(ring_iterator it, int i) noexcept { it -= i; return it; }

    template<bool C> bool operator==(const ring_iterator<RV,C>& rhs) const noexcept { return idx_ == rhs.idx_; }
    template<bool C> bool operator!=(const ring_iterator<RV,C>& rhs) const noexcept { return idx_ != rhs.idx_; }
    template<bool C> bool operator<(const ring_iterator<RV,C>& rhs) const noexcept { return idx_ < rhs.idx_; }
    template<bool C> bool operator<=(const ring_iterator<RV,C>& rhs) const noexcept { return idx_ <= rhs.idx_; }
    template<bool C> bool operator>(const ring_iterator<RV,C>& rhs) const noexcept { return idx_ > rhs.idx_; }
    template<bool C> bool operator>=(const ring_iterator<RV,C>& rhs) const noexcept { return idx_ >= rhs.idx_; }

private:
    friend RV;

    using size_type = typename RV::size_type;

    ring_iterator(size_type idx, std::conditional_t<is_const, const RV, RV> *rv) noexcept : idx_(idx), rv_(rv) {}

    size_type idx_;
    std::conditional_t<is_const, const RV, RV> *rv_;
};

} // namespace detail

} } // namespace std::experimental
