# std::ring_span<T>

This code came out of the SG14 working meeting at CppCon 2015.
Guy Davidson proposed `std::fixed_ring` and `std::dynamic_ring`:

- [P0059R0 "A proposal to add a ring adaptor to the standard library"](p0059r0.pdf)

This code proposes just a single, lower-level primitive: `std::ring_span`,
as described in the current revision of P0059:

- [P0059R1 "A proposal to add a ring span to the standard library"](p0059r1.pdf)

A `ring_span` doesn't manage its own memory nor its own objects; it is
merely a non-owning (yet mutable) *view* onto an array of existing objects,
providing the following facilities:

 - `push_back()` assigns a new value to *end() and increments end().
   If the ring-buffer is at capacity, this will cause begin() also to be
   incremented; that is, the oldest item in the ring-buffer will be
   forgotten.

 - `pop_front()` performs a customizable action and increments begin().

 - Iteration of the whole buffer (from front to back) is possible
   via the usual `begin()` and `end()` iterators, or via range-based for loop.

 - The `ring_span` itself is a lightweight value type; you can copy it
   to get a second (equivalent) view of the same objects. As with `array_view`
   and `string_view`, any operation that invalidates a pointer in the range
   on which a view was created invalidates pointers and references returned
   from the view's functions.

For the `array_view` proposal, see
[N4177 "Multidimensional bounds, index and array_view"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4177.html)
