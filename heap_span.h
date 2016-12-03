#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace std { namespace experimental {

template<class T, class Comparator = std::less<>>
class heap_span
{
public:
    using type = heap_span<T>;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;

    heap_span() = default;

    template<class ContiguousIterator>
    heap_span(ContiguousIterator begin, ContiguousIterator end, Comparator cmp = Comparator()) noexcept :
        data_(&*begin),
        size_(end - begin),
        capacity_(end - begin),
        less_(std::move(cmp))
    {}

    template<class ContiguousIterator>
    heap_span(ContiguousIterator begin, ContiguousIterator end, size_type size) noexcept :
        data_(&*begin),
        size_(size),
        capacity_(end - begin)
    {}

    reference top() noexcept { return data_[0]; }
    const_reference top() const noexcept { return data_[0]; }

    bool empty() const noexcept { return size_ == 0; }
    bool full() const noexcept { return size_ == capacity_; }
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }

    void pop()
    {
        assert(not empty());
        if (size_ == 1) {
            size_ = 0;
        } else {
            data_[0] = std::move(data_[size_ - 1]);
            --size_;
            heapify_down_();
        }
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_copy_assignable<T>::value>>
    void push(const T& value) noexcept(std::is_nothrow_copy_assignable<T>::value)
    {
        if (full()) {
            data_[0] = value;
            heapify_down_();
        } else {
            data_[size_++] = value;
            heapify_up_();
        }
    }

    template<bool b=true, typename=std::enable_if_t<b && std::is_move_assignable<T>::value>>
    void push(T&& value) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        if (full()) {
            data_[0] = std::move(value);
            heapify_down_();
        } else {
            data_[size_++] = std::move(value);
            heapify_up_();
        }
    }

    template<typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value && std::is_nothrow_move_assignable<T>::value)
    {
        if (full()) {
            data_[0] = T(std::forward<Args>(args)...);
            heapify_down_();
        } else {
            data_[size_++] = T(std::forward<Args>(args)...);
            heapify_up_();
        }
    }

    void sort()
    {
        // for the sake of argument
        std::sort(data_, data_ + size_, less_);
    }

    void make_heap()
    {
        heapify_();
    }

    void swap(heap_span& rhs) noexcept(std::__is_nothrow_swappable<Comparator>::value)
    {
        using std::swap;
        swap(data_, rhs.data_);
        swap(size_, rhs.size_);
        swap(capacity_, rhs.capacity_);
        swap(less_, rhs.less_);
    }

    friend void swap(heap_span& lhs, heap_span& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

private:
    // all private members are exposition-only

    static size_type parent_(size_type child) { return (child - 1) / 2; }
    static size_type left_child_(size_type parent) { return parent * 2 + 1; }
    static size_type right_child_(size_type parent) { return parent * 2 + 2; }

    void heapify_()
    {
        const size_type final_size = size_;
        size_ = 0;
        while (size_ != final_size) {
            ++size_;
            heapify_up_();
        }
    }

    void heapify_up_()
    {
        using std::swap;
        assert(size_ >= 1);
        size_type idx = size_ - 1;
        while (idx != 0) {
            size_type parent = parent_(idx);
            if (less_(data_[idx], data_[parent])) {
                swap(data_[idx], data_[parent]);
                idx = parent;
            } else {
                return;
            }
        }
    }

    void heapify_down_()
    {
        using std::swap;
        size_type idx = 0;
        while (right_child_(idx) < size_) {
            size_type left = left_child_(idx);
            size_type right = right_child_(idx);
            if (less_(data_[left], data_[idx])) {
                if (less_(data_[right], data_[left])) {
                    swap(data_[idx], data_[right]);
                    idx = right;
                } else {
                    swap(data_[idx], data_[left]);
                    idx = left;
                }
            } else if (less_(data_[right], data_[idx])) {
                swap(data_[idx], data_[right]);
                idx = right;
            } else {
                return;
            }
        }
        if (left_child_(idx) < size_) {
            if (less_(data_[left_child_(idx)], data_[idx])) {
                swap(data_[idx], data_[left_child_(idx)]);
            }
        }
    }

    T *data_;
    size_type size_;
    size_type capacity_;
    Comparator less_;
};

} } // namespace std::experimental
