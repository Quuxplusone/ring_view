# std::ring_view<T>

This code came out of the SG14 working meeting at CppCon 2015.
Guy Davidson proposed `std::fixed_ring` and `std::dynamic_ring`:

P0059R0 "A proposal to add a ring adaptor to the standard library"
https://github.com/WG21-SG14/SG14/blob/master/Docs/Proposals/RingProposal.pdf

This code proposes just a single, lower-level primitive: `std::ring_view`.
A `ring_view` doesn't manage its own memory nor its own objects; it is
merely a non-owning *view* onto an array of existing objects, providing
the following facilities:

 - `push_back()` assigns a new value to *end() and increments end().

 - `pop_front()` performs a customizable action and increments begin().

 - Iteration of the whole buffer (from oldest item to newest) is possible
   via the usual `begin()` and `end()` iterators, or via range-based for loop.

 - The `ring_view` itself is a lightweight value type; you can copy it
   to get a second (equivalent) view of the same objects. As with `array_view`
   and `string_view`, any operation that invalidates a pointer in the range
   on which a view was created invalidates pointers and references returned
   from the view's functions.

For the `array_view` proposal, see
N4177 "Multidimensional bounds, index and array_view"
http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4177.html
