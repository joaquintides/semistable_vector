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
  bool operator()(Iterator it) const { return true; }
};
  
struct keep_none
{
  template<typename Iterator>
  bool operator()(Iterator it) const { return false; }
};

template<typename Iterator>
struct keep_before_impl
{
  bool operator()(Iterator it) const { return it < pos; }
  Iterator pos;
};

template<typename Iterator>
keep_before_impl<Iterator> keep_before(Iterator pos) { return {pos}; }

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

template<typename Vector>
void test()
{
  using value_type = typename Vector::value_type;
  auto rng = make_range<value_type>(20);

  /* TBW */

  {
    Vector x{rng.begin(), rng.end()};
    test_stability(x, [&] { 
      auto       it = x.begin() + (std::ptrdiff_t)(rng.size() / 2);
      value_type v{}; 
      x.emplace_back(v);
      x.push_back(v);
      x.push_back(value_type{});
      x.pop_back();
      x.insert(x.end(), v);
      x.insert(x.end(), value_type{});
      x.insert(it, v);
      x.insert(it, value_type{});
      x.resize(x.size() * 2);
      x.resize(x.size() * 2, value_type{});
      x.reserve(x.capacity() * 2);
      x.shrink_to_fit();
    });
  }
}

int main()
{
  test<semistable::vector<int>>();

  return boost::report_errors();
}
