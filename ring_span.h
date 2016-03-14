#pragma once

// Reference implementation of P0059R1.

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

namespace std { namespace experimental {

namespace detail {
    template<class, bool> class ring_iterator;
} // namespace detail

template<class T>
struct null_popper {
    void operator()(T&) { }
};

template<class T>
struct default_popper {
    T operator()(T& t) { return std::move(t); }
};

template<class T>
struct copy_popper {
    copy_popper() = default;  // TODO FIXME BUG HACK: P0059R1 omits this constructor
    copy_popper(const T& t) : t_(t) {}  // TODO FIXME BUG HACK: P0059R1 omits this constructor
    copy_popper(T&& t) : t_(std::move(t)) {}
    T operator()(T& t) { T result = t; t = t_; return result; }
private:
    T t_;
};

// TODO FIXME BUG HACK: perhaps the default type here should actually be null_popper
template<class T, class Popper = default_popper<T>>
class ring_span
{
public:
    using type = ring_span<T, Popper>;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using iterator = detail::ring_iterator<ring_span, false>;
    using const_iterator = detail::ring_iterator<ring_span, true>;

    // Construct a full ring_span.
    //
    template<class ContiguousIterator>
    ring_span(ContiguousIterator begin, ContiguousIterator end, Popper p = Popper()) noexcept :
        data_(&*begin),
        size_(end - begin),
        capacity_(end - begin),
        front_idx_(0),
        popper_(std::move(p))
    {}

    // Construct a "partially full" ring_span.
    //
    template<class ContiguousIterator>
    ring_span(ContiguousIterator begin, ContiguousIterator end, ContiguousIterator first, size_type size, Popper p = Popper()) noexcept :
        data_(&*begin),
        size_(size),
        capacity_(end - begin),
        front_idx_(first - begin),
        popper_(std::move(p))
    {}

    // Notice that an iterator contains a pointer to the ring_span itself.
    // Destroying ring_span rv invalidates rv.begin(), just as with an owning container.
    //
    iterator begin() noexcept { return iterator(0, this); }
    iterator end() noexcept { return iterator(size(), this); }
    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }
    const_iterator cbegin() const noexcept { return const_iterator(0, this); }
    const_iterator cend() const noexcept { return const_iterator(size(), this); }

    // TODO FIXME BUG HACK: These aren't in P0059R1, but they should be.
    //
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
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

    // pop_front() increments the index of the begin of the ring.
    // Notice that it does not destroy anything.
    // Calling pop_front() on an empty ring is undefined,
    // in the same way as calling pop_front() on an empty vector or list is undefined.
    // Without pop_front(), you can't use ring_span as a std::queue.
    //
    auto pop_front()
    {
        assert(not empty());
        auto old_front_idx = front_idx_;
        front_idx_ = (front_idx_ + 1) % capacity_;
        --size_;
        return popper_(data_[old_front_idx]);
    }

    // TODO FIXME BUG HACK: P0059R1 omits this member function.
    auto pop_back()
    {
        assert(not empty());
        auto old_back_idx = (front_idx_ + size_ - 1) % capacity_;
        --size_;
        return popper_(data_[old_back_idx]);
    }

    // push_back() assigns a new value to the element
    // at the end of the ring, and makes that element the
    // new back of the ring. If the ring is full before
    // the call to push_back(), we rotate the indices and
    // invalidate all iterators into the ring.
    // Without push_back(), you can't use ring_span as a std::queue.
    //
    template<bool b=true, typename=std::enable_if_t<b && std::is_copy_assignable<T>::value>>
    void push_back(const T& value) noexcept(std::is_nothrow_copy_assignable<T>::value)
    {
        data_[back_idx()] = value;
        if (not full()) {
            ++size_;
        } else {
            front_idx_ = (front_idx_ + 1) % capacity_;
        }
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_move_assignable<T>::value>>
    void push_back(T&& value) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        data_[back_idx()] = std::move(value);
        if (not full()) {
            ++size_;
        } else {
            front_idx_ = (front_idx_ + 1) % capacity_;
        }
    }

    template<typename... Args>
    void emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value && std::is_nothrow_move_assignable<T>::value)
    {
        data_[back_idx()] = T(std::forward<Args>(args)...);
        if (not full()) {
            ++size_;
        } else {
            front_idx_ = (front_idx_ + 1) % capacity_;
        }
    }

    // TODO FIXME BUG HACK: P0059R1 omits this member function.
    template<bool b=true, typename=std::enable_if_t<b && std::is_copy_assignable<T>::value>>
    void push_front(const T& value) noexcept(std::is_nothrow_copy_assignable<T>::value)
    {
        front_idx_ = (front_idx_ + capacity_ - 1) % capacity_;
        data_[front_idx_] = value;
        if (not full()) {
            ++size_;
        }
    }

    // TODO FIXME BUG HACK: P0059R1 omits this member function.
    template<bool b=true, typename=std::enable_if_t<b && std::is_move_assignable<T>::value>>
    void push_front(T&& value) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        front_idx_ = (front_idx_ + capacity_ - 1) % capacity_;
        data_[front_idx_] = std::move(value);
        if (not full()) {
            ++size_;
        }
    }

    // TODO FIXME BUG HACK: P0059R1 omits this member function.
    template<typename... Args>
    void emplace_front(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value && std::is_nothrow_move_assignable<T>::value)
    {
        front_idx_ = (front_idx_ + capacity_ - 1) % capacity_;
        data_[front_idx_] = T(std::forward<Args>(args)...);
        if (not full()) {
            ++size_;
        }
    }

    // TODO FIXME BUG HACK: P0059R1 has an unconditional "noexcept" here
    void swap(ring_span& rhs) noexcept(std::__is_nothrow_swappable<Popper>::value)
    {
        using std::swap;
        swap(data_, rhs.data_);
        swap(size_, rhs.size_);
        swap(capacity_, rhs.capacity_);
        swap(front_idx_, rhs.front_idx_);
        swap(popper_, rhs.popper_);  // TODO FIXME BUG HACK: this is inefficient unless we specialize swap for the standard poppers
    }

    friend void swap(ring_span& lhs, ring_span& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

private:
    friend class detail::ring_iterator<ring_span, true>;
    friend class detail::ring_iterator<ring_span, false>;

    reference at(size_type i) noexcept { return data_[(front_idx_ + i) % capacity_]; }
    const_reference at(size_type i) const noexcept { return data_[(front_idx_ + i) % capacity_]; }

    size_type back_idx() const noexcept { return (front_idx_ + size_) % capacity_; }

    T *data_;
    size_type size_;
    size_type capacity_;
    size_type front_idx_;
    Popper popper_;
};

// TODO FIXME BUG HACK: P0059R1 doesn't define the semantics of this function.
// TODO FIXME BUG HACK: P0059R1 has a default value for "Popper" here, which is useless.
// TODO FIXME BUG HACK: Why is this a free function?
template<typename T, class Popper>
bool try_push_back(ring_span<T, Popper>& r, const T& value) noexcept(noexcept(r.push_back(value)))
{
    if (r.full()) {
        return false;
    } else {
        r.push_back(value);
        return true;
    }
}

template<typename T, class Popper>
bool try_push_back(ring_span<T, Popper>& r, T&& value) noexcept(noexcept(r.push_back(std::move(value))))
{
    if (r.full()) {
        return false;
    } else {
        r.push_back(std::move(value));
        return true;
    }
}

template<typename T, class Popper, class... Args>
bool try_emplace_back(ring_span<T, Popper>& r, Args&&... args) noexcept(noexcept(r.emplace_back(std::forward<Args>(args)...)))
{
    if (r.full()) {
        return false;
    } else {
        r.emplace_back(std::forward<Args>(args)...);
        return true;
    }
}

// TODO FIXME BUG HACK: I don't know what the return value of this function is supposed to be.
// Probably should be something like std::optional<decltype(r.pop_front())>.
template<typename T, class Popper>
auto try_pop_front(ring_span<T, Popper>& r) noexcept(noexcept(r.pop_front()))
{
    if (r.empty()) {
        return false;
    } else {
        r.pop_front();  // TODO FIXME BUG HACK: and throw it away??
        return true;
    }
}

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

    ring_iterator() = default;  // TODO: P0059R1 is missing this default constructor?

    reference operator*() const noexcept { return rv_->at(idx_); }
    ring_iterator& operator++() noexcept { ++idx_; return *this; }
    ring_iterator operator++(int) noexcept { auto r(*this); ++*this; return r; }
    ring_iterator& operator--() noexcept { ++idx_; return *this; }
    ring_iterator operator--(int) noexcept { auto r(*this); ++*this; return r; }

    friend ring_iterator& operator+=(ring_iterator& it, int i) noexcept { it.idx_ += i; return it; }
    friend ring_iterator& operator-=(ring_iterator& it, int i) noexcept { it.idx_ -= i; return it; }
    friend ring_iterator operator+(ring_iterator it, int i) noexcept { it += i; return it; }
    friend ring_iterator operator-(ring_iterator it, int i) noexcept { it -= i; return it; }

    // TODO FIXME BUG HACK: P0059R1 is missing four of these operators
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

} } //namespace std::experimental
