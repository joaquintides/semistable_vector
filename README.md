# Semistable vector

```cpp
#include <iostream>
#include "semistable/vector.hpp"

int main()
{
  semistable::vector<int> x = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto it = x.begin() + 5;
  std::cout << *it << "\n"; // prints 5

  x.erase(x.begin());       // erases first element

  std::cout << *it << "\n"; // prints 5 again!
}
```

`std::vector` stores its contents in a contiguous block of memory, so mid insertions and
erasures invalidate references and iterators to previous elements. `semistable::vector`
is called _semistable_ in the sense that, while references are still unstable, its iterators
correctly track elements in situations like the above.

`semistable::vector` provides the exact same API as `std::vector` with the extra
guarantee of iterator stability (including `end()`). The library is header-only and depends solely on
[Boost.Config](https://www.boost.org/doc/libs/latest/libs/config/doc/html/index.html).

## Implementation

TBW

## Performance

The graph shows execution times of the following operations:

* traversal with `for_each`,
* repeated insertion at the end,
* `erase_if` of odd elements,
* sorting of elements,

for `std::vector<int>`, `std::list<int>` and `semistable::vector<int>` with 0.5M elements.
Results are normalized to the execution time of `std::vector`.
