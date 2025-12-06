/* Copyright 2025 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#if !defined(NDEBUG)
#define SEMISTABLE_ENABLE_INVARIANT_CHECKING
#endif

#include <algorithm>
#include <boost/config.hpp>
#include <boost/core/lightweight_test.hpp>
#include <memory>
#include <semistable/vector.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template<typename T>
struct tracked
{
  tracked(const T& x_, int copy_count_, int move_count_):
    x{x_}, copy_count{copy_count_}, move_count{move_count_} {}
  tracked(const T& x_):
    x{x_}, copy_count{1}, move_count{0} {}
  tracked(T&& x_):
    x{x_}, copy_count{0}, move_count{1} {}
  tracked(const tracked& x_):
    x{x_.x},
    copy_count{x_.copy_count + 1}, move_count{x_.move_count} {}
  tracked(tracked&& x_):
    x{std::move(x_.x)},
    copy_count{x_.copy_count}, move_count{x_.move_count + 1} {}

  T x;
  int copy_count = 0;
  int move_count = 0;
};

template<typename Vector, typename U>
struct rebind_value_type;

template<
  template<typename...> class Vector, typename T, typename Allocator,
  typename U
>
struct rebind_value_type<Vector<T, Allocator>, U>
{
  using type = Vector<
    U, 
    typename std::allocator_traits<Allocator>::template rebind_alloc<U>
  >;
};

template<typename Vector, typename U>
using rebind_value_type_t = typename rebind_value_type<Vector, U>::type;

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

#if BOOST_CXX_VERSION >= 202002L

/* 
 https://bannalia.blogspot.com/2016/09/compile-time-checking-existence-of.html
 */

namespace from_range_t_fallback {
struct from_range_t{};
struct hook{};
}

namespace std {
template<>
struct hash<::from_range_t_fallback::hook>
{
  using from_range_t_type = decltype([] {
    using namespace from_range_t_fallback;
    return from_range_t{};
  }());
};
}

using from_range_t_or_else = 
  std::hash<from_range_t_fallback::hook>::from_range_t_type;

#else

using from_range_t_or_else = void*;

#endif

template<
  typename Vector, typename FromRangeT, typename R,
  typename std::enable_if<
    std::is_constructible<
      Vector, 
      FromRangeT, R&&, const typename Vector::allocator_type&
    >::value
  >::type* = nullptr
>
void test_range_ctor_impl(FromRangeT, const R& rng, int)
{
  Vector x{FromRangeT{}, rng}, 
         y{FromRangeT{}, rng, typename Vector::allocator_type{}};
  test_equal(x, rng);
  test_equal(y, rng);
  BOOST_TEST(false);
}

template<typename Vector, typename FromRangeT, typename R>
void test_range_ctor_impl(FromRangeT, const R& rng, ...)
{
}

template<typename Vector, typename R>
void test_range_ctor(const R& rng)
{
  test_range_ctor_impl<Vector>(from_range_t_or_else{}, rng, 0);
}

template<
  typename Vector, typename R,
  typename std::enable_if<
    sizeof(
      std::declval<Vector>().assign_range(std::declval<const R&>()), 0) != 0
  >::type* =nullptr
>
void test_assign_range_impl(const R& rng, int)
{
  Vector x;
  x.assign_range(rng);
  test_equal(x, rng);
}

template<typename Vector, typename R>
void test_assign_range_impl(const R&, ...)
{
}

template<typename Vector, typename R>
void test_assign_range(const R& rng)
{
  test_assign_range_impl<Vector>(rng, 0);
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

template<typename T> void avoid_unused_local_typedef() {}

template<typename Vector>
void test()
{
  using value_type = typename Vector::value_type;
  using allocator_type= typename Vector::allocator_type;
  using pointer = typename Vector::pointer;
  using const_pointer = typename Vector::const_pointer;
  using reference = typename Vector::reference;
  using const_reference = typename Vector::const_reference;
  using size_type = typename Vector::size_type;
  using difference_type = typename Vector::difference_type;
  using iterator = typename Vector::iterator;
  using const_iterator = typename Vector::const_iterator;
  using reverse_iterator = typename Vector::reverse_iterator;
  using const_reverse_iterator =
    typename Vector::const_reverse_iterator;

  avoid_unused_local_typedef<pointer>();
  avoid_unused_local_typedef<const_pointer>();
  avoid_unused_local_typedef<reference>();
  avoid_unused_local_typedef<const_reference>();
  avoid_unused_local_typedef<size_type>();
  avoid_unused_local_typedef<difference_type>();
  avoid_unused_local_typedef<reverse_iterator>();
  avoid_unused_local_typedef<const_reverse_iterator>();

  const allocator_type              al{};
  auto                              rng = make_range<value_type>(20);
  std::initializer_list<value_type> il{rng[5], rng[1], rng[7]};
  std::vector<value_type>           zeros(7, value_type());
  std::vector<value_type>           repeated(10, rng[10]);

  /* construct/copy/destroy */

  {
    Vector x;
    BOOST_TEST(x.empty());
  }
  {
    Vector x{al};
    BOOST_TEST(x.empty());
  }
  {
    Vector x(zeros.size()), 
#if defined(_LIBCPP_VERSION) && BOOST_CXX_VERSION < 201402L
           y{zeros.size(), value_type(), al};
#else      
           y{zeros.size(), al};
#endif

    test_equal(x, zeros);
    BOOST_TEST(x == y);
  }
  {
    Vector x(repeated.size(), repeated[0]),
           y{repeated.size(), repeated[0], al};
    test_equal(x, repeated);
    BOOST_TEST(x == y);
  }
  {
    /* [sequence.reqmts/69.1] */

    Vector x(20, 20);
    BOOST_TEST_EQ(x.size(), 20);
  }
  {
    Vector x{rng.begin(), rng.end()}, y{rng.begin(), rng.end(), al};
    test_equal(x, rng);
    BOOST_TEST(x == y);
  }
  {
    test_range_ctor<Vector>(rng);
  }
  {
    const Vector x{rng.begin(), rng.end()};
    Vector       y{x}, z{y, al};
    BOOST_TEST(x == y);
    BOOST_TEST(x == z);
  }
  {
    Vector x{rng.begin(), rng.end()};
    Vector y{std::move(x)};
    BOOST_TEST(x.empty());
    test_equal(y, rng);

    Vector z{std::move(y), al};
    BOOST_TEST(y.empty());
    test_equal(z, rng);
  }
  {
    Vector x{il}, y{il, al};
    test_equal(x, il);
    BOOST_TEST(x == y);
  }
  {
    Vector  x{rng.begin(), rng.end()}, y;
    Vector& ry = (y = x);
    BOOST_TEST(&ry == &y);
    BOOST_TEST(x == y);
  }
  {
    Vector  x{rng.begin(), rng.end()}, y;
    Vector& ry = (y = std::move(x));
    BOOST_TEST(&ry == &y);
    BOOST_TEST(x.empty());
    test_equal(y, rng);
  }
  {
    Vector  x;
    Vector& rx = (x = il);
    BOOST_TEST(&rx == &x);
    test_equal(x, il);
  }
  {
    Vector x;
    x.assign(rng.begin(), rng.end());
    test_equal(x, rng);
  }
  {
    test_assign_range<Vector>(rng);
  }
  {
    Vector x;
    x.assign(repeated.size(), repeated[0]);
    test_equal(x, repeated);
  }
  {
    Vector x;
    x.assign(il);
    test_equal(x, il);
  }
  {
    const Vector x{al};
    BOOST_TEST(x.get_allocator() == al);
  }

  /* iterators */

  {
    Vector        x{rng.begin(), rng.end()};
    const Vector& cx=x;

    BOOST_TEST_EQ(std::addressof(*x.begin()), x.data());
    BOOST_TEST_EQ(std::addressof(*cx.begin()), x.data());
    BOOST_TEST_EQ(std::addressof(*(x.end() - 1)), x.data() + x.size() - 1);
    BOOST_TEST_EQ(std::addressof(*(cx.end() - 1)), x.data() + x.size() - 1);
    BOOST_TEST_EQ(std::addressof(*(x.end() - 1)), x.data() + x.size() - 1);
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
  {
    /* operator-> */

    rebind_value_type_t<Vector, std::pair<int, int>> x;
    x.emplace_back(18, 42);
    BOOST_TEST_EQ(x.begin()->first, 18);
    BOOST_TEST_EQ(x.cbegin()->second, 42);
  }

  /* capacity */

  {
    Vector        x;
    const Vector& cx = x;

    x.reserve(1000);
    x.insert(x.end(), rng.begin(), rng.end());
    BOOST_TEST(!cx.empty());
    BOOST_TEST_EQ(cx.size(), rng.size());
    BOOST_TEST_GT(cx.max_size(), 0);
    BOOST_TEST_GE(cx.capacity(), 1000);

    x.resize(rng.size() / 2);
    BOOST_TEST_EQ(cx.size(), rng.size() / 2);
    x.resize(rng.size());
    BOOST_TEST_EQ(cx.size(), rng.size());
    BOOST_TEST((
      std::count(x.begin() + rng.size() / 2, x.end(), value_type()) ==
      rng.size()  - rng.size() / 2));
    x.resize(2 * rng.size(), rng[5]);
    BOOST_TEST((
      std::count(x.begin() + rng.size() , x.end(), rng[5]) == rng.size()));

    Vector x2 = x;
    x.shrink_to_fit();
    BOOST_TEST(x == x2);
    BOOST_TEST_EQ(cx.size(), 2 * rng.size());
    BOOST_TEST_GE(cx.capacity(), 2 * rng.size());
  }

  /* element access */

  {
    Vector        x{rng.begin(), rng.end()};
    const Vector& cx = x;
    auto          n = x.size();
    auto          data = x.data();

    BOOST_TEST_EQ(std::addressof(x[n / 2]), std::addressof(data[n / 2]));
    BOOST_TEST_EQ(std::addressof(cx[n / 2]), std::addressof(data[n / 2]));
    BOOST_TEST_EQ(std::addressof(x.at(n / 2)), std::addressof(data[n / 2]));
    BOOST_TEST_EQ(std::addressof(cx.at(n / 2)), std::addressof(data[n / 2]));
    BOOST_TEST_THROWS((void)x.at(n), std::out_of_range);
    BOOST_TEST_THROWS((void)cx.at(n), std::out_of_range);
    BOOST_TEST_EQ(std::addressof(x.front()), std::addressof(data[0]));
    BOOST_TEST_EQ(std::addressof(cx.front()), std::addressof(data[0]));
    BOOST_TEST_EQ(std::addressof(x.back()), std::addressof(data[n - 1]));
    BOOST_TEST_EQ(std::addressof(cx.back()), std::addressof(data[n - 1]));
  }

  /* data access */

  {
    Vector        x{rng.begin(), rng.end()};
    const Vector& cx = x;
    auto          n = x.size();
    auto          data = x.data();
    auto          cdata = cx.data();

    BOOST_TEST_EQ(data, cdata);

    data[n/2] += value_type(1);
    BOOST_TEST(x[n/2] == data[n/2]);
  }

  /* modifiers */

  using tracked_vector = rebind_value_type_t<Vector, tracked<value_type>>;
  using tracked_value_type = tracked<value_type>;

  {
    tracked_vector     x;
    tracked_value_type v(value_type{}, 0, 0);

    x.emplace_back(v.x);
    BOOST_TEST(x.back().x == v.x);
    BOOST_TEST_EQ(x.back().copy_count, 1);

    v.x += value_type(1);
    x.emplace_back(std::move(v.x));
    BOOST_TEST(x.back().x == v.x);
    BOOST_TEST_EQ(x.back().move_count, 1);

    v.x += value_type(1);
    x.push_back(v);
    BOOST_TEST(x.back().x == v.x);
    BOOST_TEST_EQ(x.back().copy_count, 1);

    v.x += value_type(1);
    x.push_back(std::move(v));
    BOOST_TEST(x.back().x == v.x);
    BOOST_TEST_EQ(x.back().move_count, 1);

    auto  s = x.size();
    auto& r = *(x.end() - 2);
    x.pop_back();
    BOOST_TEST_EQ(x.size(), s - 1);
    BOOST_TEST_EQ(std::addressof(x.back()), std::addressof(r));
  }

  // TODO: rest of API
}

int main()
{
  /* detect potential bugs in relied-on stdlib or tests themselves */
  test<std::vector<int>>(); 

  test<semistable::vector<int>>();
  test<semistable::vector<std::size_t>>();

  return boost::report_errors();
}