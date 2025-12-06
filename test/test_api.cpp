/* Copyright 2025 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#if !defined(NDEBUG)
#define SEMISTABLE_ENABLE_INVARIANT_CHECKING
#endif

#include <algorithm>
#include <boost/core/lightweight_test.hpp>
#include <semistable/vector.hpp>
#include <vector>

template<typename T>
std::vector<T> make_range(std::size_t n)
{
  std::vector<T> res;
  T i = T();
  while(--n) {
    res.push_back(i);
    i += T(1);
  }
  return res;
}

template<typename Container1, typename Container2>
void test_equal(const Container1& x, const Container2& y)
{
  BOOST_TEST_EQ(x.size(), y.size());
  BOOST_TEST(std::equal(x.begin(), x.end(), y.begin()));
}

template<typename Iterator, typename T>
void test_traversal(Iterator first, Iterator last, T* data)
{
  std::ptrdiff_t n = 0;
  for(auto it = first; it != last; ++it, ++n)
  {
    BOOST_TEST(first[n] == data[n]);
    BOOST_TEST(*it == data[n]);
    BOOST_TEST_EQ(it - first, n);
    BOOST_TEST_EQ(first - it, -n);
    BOOST_TEST(first + n == it);
    BOOST_TEST(n + first == it);
    BOOST_TEST(it - n == first);
    BOOST_TEST((first == it) == (0 == n));
    BOOST_TEST((first != it) == (0 != n));
    BOOST_TEST((first < it) == (0 < n));
    BOOST_TEST((first > it) == (0 > n));
    BOOST_TEST((first >= it) == (0 >= n));
    BOOST_TEST((first <= it) == (0 <= n));

    auto it1 = it, it2 = ++it1, it3 = --it2, it4 = it3++, it5 = it3-- ;
    BOOST_TEST(it1 == it + 1);
    BOOST_TEST(it2 == it);
    BOOST_TEST(it3 == it);
    BOOST_TEST(it4 == it);
    BOOST_TEST(it5 == it + 1);
  }
}

template<typename SemistableVector>
void test()
{
  using value_type = typename SemistableVector::value_type;
  using allocator_type= typename SemistableVector::allocator_type;
  using pointer = typename SemistableVector::pointer;
  using const_pointer = typename SemistableVector::const_pointer;
  using reference = typename SemistableVector::reference;
  using const_reference = typename SemistableVector::const_reference;
  using size_type = typename SemistableVector::size_type;
  using difference_type = typename SemistableVector::difference_type;
  using iterator = typename SemistableVector::iterator;
  using const_iterator = typename SemistableVector::const_iterator;
  using reverse_iterator = typename SemistableVector::reverse_iterator;
  using const_reverse_iterator =
    typename SemistableVector::const_reverse_iterator;

  const allocator_type              al{};
  auto                              rng = make_range<value_type>(20);
  std::initializer_list<value_type> il{rng[5], rng[1], rng[7]};
  std::vector<value_type>           zeros(7, value_type());
  std::vector<value_type>           repeated(10, rng[10]);

  /* construct/copy/destroy */

  {
    SemistableVector x;
    BOOST_TEST(x.empty());
  }
  {
    SemistableVector x{al};
    BOOST_TEST(x.empty());
  }
  {
    SemistableVector x(zeros.size()), y{zeros.size(), al};
    test_equal(x, zeros);
    BOOST_TEST(x == y);
  }
  {
    SemistableVector x(repeated.size(), repeated[0]),
                     y{repeated.size(), repeated[0], al};
    test_equal(x, repeated);
    BOOST_TEST(x == y);
  }
  {
    /* [sequence.reqmts/69.1] */

    SemistableVector x(20, 20);
    BOOST_TEST_EQ(x.size(), 20);
  }
  {
    SemistableVector x{rng.begin(), rng.end()}, y{rng.begin(), rng.end(), al};
    test_equal(x, rng);
    BOOST_TEST(x == y);
  }
  {
    // TODO: from_range_t ctor
  }
  {
    const SemistableVector x{rng.begin(), rng.end()};
    SemistableVector y{x}, z{y, al};
    BOOST_TEST(x == y);
    BOOST_TEST(x == z);
  }
  {
    SemistableVector x{rng.begin(), rng.end()};
    SemistableVector y{std::move(x)};
    BOOST_TEST(x.empty());
    test_equal(y, rng);

    SemistableVector z{std::move(y), al};
    BOOST_TEST(y.empty());
    test_equal(z, rng);
  }
  {
    SemistableVector x{il}, y{il, al};
    test_equal(x, il);
    BOOST_TEST(x == y);
  }
  {
    SemistableVector  x{rng.begin(), rng.end()}, y;
    SemistableVector& ry = (y = x);
    BOOST_TEST(&ry == &y);
    BOOST_TEST(x == y);
  }
  {
    SemistableVector  x{rng.begin(), rng.end()}, y;
    SemistableVector& ry = (y = std::move(x));
    BOOST_TEST(&ry == &y);
    BOOST_TEST(x.empty());
    test_equal(y, rng);
  }
  {
    SemistableVector  x;
    SemistableVector& rx = (x = il);
    BOOST_TEST(&rx == &x);
    test_equal(x, il);
  }
  {
    SemistableVector x;
    x.assign(rng.begin(), rng.end());
    test_equal(x, rng);
  }
  {
    // TODO: assign_range
  }
  {
    SemistableVector x;
    x.assign(repeated.size(), repeated[0]);
    test_equal(x, repeated);
  }
  {
    SemistableVector x;
    x.assign(il);
    test_equal(x, il);
  }
  {
    const SemistableVector x{al};
    BOOST_TEST(x.get_allocator() == al);
  }

  /* iterators */

  {
    SemistableVector        x{rng.begin(), rng.end()};
    const SemistableVector& cx=x;

    BOOST_TEST_EQ(x.begin().operator->(), x.data());
    BOOST_TEST_EQ(cx.begin().operator->(), x.data());
    BOOST_TEST_EQ(x.end().operator->(), x.data() + x.size());
    BOOST_TEST_EQ(cx.end().operator->(), x.data() + x.size());
    BOOST_TEST_EQ(x.end().operator->(), x.data() + x.size());
    BOOST_TEST(x.rbegin().base() == x.end());
    BOOST_TEST(cx.rbegin().base() == cx.end());
    BOOST_TEST(x.rend().base() == x.begin());
    BOOST_TEST(cx.rend().base() == cx.begin());
    BOOST_TEST(cx.cbegin() == cx.begin());
    BOOST_TEST(cx.cend() == cx.end());
    BOOST_TEST(cx.crbegin() == cx.rbegin());
    BOOST_TEST(cx.crend() == cx.rend());

    iterator       it = x.begin(), it2 = x.end();
    const_iterator cit = it;
    BOOST_TEST(cit == it);
    cit = it2;
    BOOST_TEST(cit == it2);
    it = it2;
    BOOST_TEST(it == it2);

    test_traversal(x.begin(), x.end(), x.data());
    test_traversal(x.cbegin(), x.cend(), x.data());
  }

  // TODO: rest of API
}

int main()
{
  test<semistable::vector<int>>();
  test<semistable::vector<std::size_t>>();
  return boost::report_errors();
}