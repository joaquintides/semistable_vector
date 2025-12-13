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
#include <type_traits>
#include <utility>
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

struct keep_all
{
  template<typename Iterator>
  bool operator()(Iterator) const { return true; }
};
  
struct keep_none
{
  template<typename Iterator>
  bool operator()(Iterator) const { return false; }
};

template<typename Vector, typename F, typename Keep = keep_all>
void test_stability(Vector&& x, F f, Keep keep = {})
{
  using vector_type = typename std::remove_const<
    typename std::remove_reference<Vector>::type>::type;
  using value_type = typename vector_type::value_type;
  using const_iterator = typename vector_type::const_iterator;

  const_iterator                                     last = x.end();
  std::vector<std::pair<const_iterator, value_type>> kept;

  for(auto first = x.begin(); first != last; ++first) {
    if(keep(first)) kept.push_back({first, *first});
  }

  f();

  for(const auto& p: kept) BOOST_TEST_EQ(*(p.first), p.second);
  BOOST_TEST(last == x.end());
}

template<
  typename Vector, typename R,
  typename = typename std::enable_if<
    sizeof(
      std::declval<Vector>().append_range(std::declval<const R&>()), 0) != 0
  >::type
>
void append_range_impl(Vector& x, const R& rng, int)
{
  x.append_range(rng);
  BOOST_TEST(false);
}

template<typename Vector, typename R>
void append_range_impl(Vector&, const R&, ...)
{
}

template<typename Vector, typename R>
void append_range(Vector& x, const R& rng)
{
  append_range_impl(x, rng, 0);
}

template<
  typename Vector, typename R,
  typename = typename std::enable_if<
    sizeof(
      std::declval<Vector>().insert_range(
        std::declval<typename Vector::const_iterator>(),
        std::declval<const R&>()), 0) != 0
  >::type
>
void insert_range_impl(
  Vector& x, typename Vector::const_iterator pos, const R& rng, int)
{
  x.insert_range(pos, rng);
  BOOST_TEST(false);
}

template<typename Vector, typename R>
void insert_range_impl(
  Vector&, typename Vector::const_iterator, const R&, ...)
{
}

template<typename Vector, typename R>
void insert_range(Vector& x, typename Vector::const_iterator pos, const R& rng)
{
  insert_range_impl(x, pos, rng, 0);
}

template<typename Vector>
void test()
{
  using value_type = typename Vector::value_type;
  using iterator = typename Vector::iterator;

  auto                              rng = make_range<value_type>(20);
  std::initializer_list<value_type> il{rng[5], rng[1], rng[7]};

  /* modifiers and capacity */

  {
    Vector x{rng.begin(), rng.end()}, y;
    test_stability(x, [&] { 
      value_type v = *std::max_element(rng.begin(), rng.end()) + 1;
      x.emplace_back(v);
      x.push_back(v);
      x.push_back(value_type{v});
      append_range(x, rng);
      x.emplace(x.begin(), v);
      x.insert(x.end(), v);
      x.insert(x.end(), value_type{v});
      x.insert(x.begin(), v);
      x.insert(x.begin() + (std::ptrdiff_t)(x.size() / 2), value_type{v});
      x.insert(x.begin() + (std::ptrdiff_t)(x.size() / 3), 10, v);
      x.insert(
        x.begin() + (std::ptrdiff_t)(x.size() / 4), rng.begin(), rng.end());
      insert_range(x, x.begin() + (std::ptrdiff_t)(x.size() / 5), rng);
      x.insert(x.begin() + (std::ptrdiff_t)(x.size() / 6), il);
      x.resize(x.size() * 2);
      x.resize(x.size() * 2, v);
      x.resize(x.size() / 2);
      x.reserve(x.capacity() * 2);
      x.shrink_to_fit();
      x.pop_back();
      x.erase(std::find(x.begin(), x.end(), v));
      x.erase( /* original elements are all before size() / 2 */
        x.begin() + (std::ptrdiff_t)(x.size() / 2),
        x.begin() + (std::ptrdiff_t)(x.size() * 3 / 4));
    });
  }

  /* erasure */

  {
    Vector x{rng.begin(), rng.end()};
    test_stability(x, [&] {
      erase_if(x, [] (const value_type& v) { return v % 2 == 0; });
    },
    [] (const iterator it) { return *it % 2 != 0;});

    x.assign(rng.begin(), rng.end());
    test_stability(x, [&] {
      erase_if(x, [] (const value_type& v) { return v % 3 < 2; });
    },
    [] (const iterator it) { return *it % 3 >= 2; });

    x.assign(rng.begin(), rng.end());
    x.insert(x.end(), rng.begin(), rng.end());
    test_stability(x, [&] {
      erase(x, rng[0]);
    },
    [&] (const iterator it) { return *it != rng[0]; });
  }
}

int main()
{
  test<semistable::vector<int>>();

  return boost::report_errors();
}
