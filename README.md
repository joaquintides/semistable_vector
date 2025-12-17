# A proof of concept of a semistable vector container

Branch [feature/monothread](https://github.com/joaquintides/semistable_vector/tree/feature/monothread):
Monothread version of `semistable::vector` internally using
[`boost::local_shared_ptr`](https://www.boost.org/doc/libs/latest/libs/smart_ptr/doc/html/smart_ptr.html#local_shared_ptr)
instead of `std::shared_ptr`.
