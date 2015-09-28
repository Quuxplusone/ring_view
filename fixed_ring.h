#pragma once

#include "ring_view.h"
#include <array>
#include <cstddef>

// An implementation of fixed_ring<T,N> as specified in Guy Davidson's
// P0059R0 "A proposal to add a ring adaptor to the standard library".
//
template<class T, std::size_t Capacity, class Container = std::array<T, Capacity>>
class fixed_ring
{
public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;

    // TODO: the proposal doesn't mention these two typedefs
    using pointer = typename Container::pointer;
    using const_pointer = typename Container::const_pointer;

    // TODO: the proposal mentions these four typedefs, but never uses them,
    // since fixed_ring<T> is not iterable.
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using reverse_iterator = typename Container::reverse_iterator;
    using const_reverse_iterator = typename Container::const_reverse_iterator;

    fixed_ring() noexcept(std::is_nothrow_default_constructible<Container>::value) :
        // TODO: proposal says is_nothrow_default_constructible<T>
        ctr_(),
        rv_(ctr_.begin(), ctr_.end(), ctr_.begin(), 0)
    {}

    // TODO: this should probably be disabled if T isn't copy-constructible
    fixed_ring(const fixed_ring& rhs) noexcept(std::is_nothrow_copy_constructible<Container>::value) :
        fixed_ring(&*rhs.rv_.begin() - rhs.ctr_.begin(), rhs.rv_.size(), rhs.ctr_)
    {}

    // TODO: this should probably be disabled if T isn't move-constructible
    // TODO: this move-constructs a whole new std::array, an expensive default behavior IMHO
    fixed_ring(fixed_ring&& rhs) noexcept(std::is_nothrow_move_constructible<Container>::value) :
        fixed_ring(&*rhs.rv_.begin() - rhs.ctr_.begin(), rhs.rv_.size(), std::move(rhs.ctr_))
    {}

    fixed_ring(const container_type& rhs) noexcept(std::is_nothrow_copy_constructible<Container>::value) :
        fixed_ring(0, rhs.size(), rhs)
    {}

    fixed_ring(container_type&& rhs) noexcept(std::is_nothrow_move_constructible<Container>::value) :
        fixed_ring(0, rhs.size(), std::move(rhs))
    {}

    fixed_ring& operator=(const fixed_ring& rhs) noexcept(std::is_nothrow_copy_assignable<Container>::value)
    {
        ctr_ = rhs.ctr_;
        auto first_idx = (&*rhs.rv_.begin() - rhs.ctr_.begin());
        rv_ = ring_view<T>(ctr_.begin(), ctr_.end(), ctr_.begin() + first_idx, rhs.rv_.size());
        return *this;
    }

    fixed_ring& operator=(fixed_ring&& rhs) noexcept(std::is_nothrow_copy_assignable<Container>::value)
    {
        auto first_idx = (&*rhs.rv_.begin() - rhs.ctr_.begin());
        ctr_ = std::move(rhs.ctr_);
        rv_ = ring_view<T>(ctr_.begin(), ctr_.end(), ctr_.begin() + first_idx, rhs.rv_.size());
        return *this;
    }

    void push(const value_type& v) { rv_.push_back(v); }
    void push(value_type&& v) { rv_.push_back(std::move(v)); }

    template<class... Args> bool emplace(Args&&... args)
    {
        rv_.push(T(std::forward<Args>(args)...));
        return true;  // TODO: what are the semantics of this return value?
    }

    bool try_push(const value_type& v) { return rv_.full() ? false : (rv_.push_back(v), true); }
    bool try_push(value_type&& v) { return rv_.full() ? false : (rv_.push_back(std::move(v)), true); }

    template<class... Args>
    bool try_emplace(Args&&... args) { return rv_.full() ? false : (rv_.push_back(T(std::forward<Args>(args)...)), true); }

    void pop() noexcept { rv_.pop_front(); }
    bool empty() const noexcept { return rv_.empty(); }
    size_type size() const noexcept { return rv_.size(); }

    // TODO: the proposal doesn't mention this member function
    constexpr size_type capacity() const noexcept { return Capacity; }

    reference front() noexcept { return rv_.front(); }
    const_reference front() const noexcept { return rv_.front(); }
    reference back() noexcept { return rv_.back(); }
    const_reference back() const noexcept { return rv_.back(); }

    void swap(fixed_ring& rhs) noexcept
    {
        using std::swap;
        swap(ctr_, rhs.ctr_);
        swap(rv_, rhs.rv_);
    }

private:
    template<class... Args>
    fixed_ring(size_type first_idx, size_type size, Args&&... args) noexcept(Container(std::forward<Args>(args)...)) :
        ctr_(std::forward<Args>(args)...),
        rv_(ctr_.begin(), ctr_.end(), ctr_.begin() + first_idx, size)
    {}

    Container ctr_;
    ring_view<T> rv_;
};
