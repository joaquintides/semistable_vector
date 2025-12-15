/* Performance of ops with semistable::vector vs. std::vector and std::list.
 * 
 * Copyright 2025 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>

std::chrono::high_resolution_clock::time_point measure_start, measure_pause;

template<typename F>
double measure(F f)
{
  using namespace std::chrono;

  static const int              num_trials = 10;
  static const milliseconds     min_time_per_trial(200);
  std::array<double,num_trials> trials;

  for(int i = 0; i < num_trials; ++i) {
    int                               runs = 0;
    high_resolution_clock::time_point t2;
    volatile decltype(f())            res; /* to avoid optimizing f() away */

    measure_start = high_resolution_clock::now();
    do{
      res = f();
      ++runs;
      t2 = high_resolution_clock::now();
    }while(t2 - measure_start<min_time_per_trial);
    trials[i] =
      duration_cast<duration<double>>(t2 - measure_start).count() / runs;
  }

  std::sort(trials.begin(), trials.end());
  return std::accumulate(
    trials.begin() + 2, trials.end() - 2, 0.0)/(trials.size() - 4);
}

void pause_timing()
{
  measure_pause = std::chrono::high_resolution_clock::now();
}

void resume_timing()
{
  measure_start += std::chrono::high_resolution_clock::now() - measure_pause;
}

#include <boost/type_index.hpp>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <list>
#include <random>
#include <semistable/vector.hpp>
#include <vector>

template<typename Container>
Container make()
{
  constexpr std::size_t N = 500'000;

  Container                          c;
  std::uniform_int_distribution<int> dist;
  std::mt19937_64                    gen(34862);

  for(std::size_t i = 0; i < N; ++i) c.insert(c.end(), dist(gen));
  return c;
}

template<
  typename Container1, typename Container2, typename F1, typename F2 = F1
>
void sanity_check(F1 f1, F2 f2 = {})
{
  Container1 c1 = make<Container1>();
  Container2 c2 = make<Container2>();

  if(f1(c1) != f2(c2) ||
     c1.size() != c2.size() ||
     !std::equal(c1.begin(), c1.end(), c2.begin())) {
    std::cerr << "sanity check failed\n";
    std::exit(EXIT_FAILURE);
  }
}

template<typename Container, typename F>
double test(F f, double base = 0.0)
{
  Container c = make<Container>();

  auto res = measure([&] {
    pause_timing();
    auto c2 = c;
    resume_timing();
    return f(c2);
  });

  auto str = boost::typeindex::type_id<Container>().pretty_name();
  auto pos1 = str.find(' ');
  auto pos2 = str.find('<');
  auto name = str.substr(pos1 + 1, pos2 - pos1 - 1);

  std::cout << std::setw(20) << (name + ": ") << res;
  if(base != 0.0) std::cout << "\t(" << res / base << ")";
  std::cout << "\n";

  return res;
}

int main()
{
  auto sort = [] (auto& c)
  {
    std::sort(c.begin(), c.end());
    return *(c.begin());
  };
  auto list_sort = [] (auto& c)
  {
    c.sort();
    return *(c.begin());
  };
  auto for_each = [] (const auto& c)
  {
    unsigned int res=0;
    std::for_each(c.begin(), c.end(), [&] (auto x) { 
      res += (unsigned int)x; 
    });
    return res;
  };
  auto insert = [](const auto& c)
  {
    using container_type = 
      std::remove_const_t<std::remove_reference_t<decltype(c)>>;
    container_type c2;
    for(const auto& x: c) c2.insert(c2.end(), x);
    return c.size();
  };
  auto erase_if_ = [] (auto& c)
  {
    erase_if(c, [] (auto x) { return x % 2 != 0; });
    return c.size();
  };

  using vector = std::vector<int>;
  using list = std::list<int>;
  using semistable_vector = semistable::vector<int>;

  sanity_check<vector, semistable_vector>(for_each);
  sanity_check<vector, list>(for_each);
  sanity_check<vector, semistable_vector>(insert);
  sanity_check<vector, list>(insert);
  sanity_check<vector, semistable_vector>(erase_if_);
  sanity_check<vector, list>(erase_if_);
  sanity_check<vector, semistable_vector>(sort);
  sanity_check<vector, list>(sort, list_sort);

  double base;

  std::cout << "for_each\n";
  base = test<vector>(for_each);
  test<list>(for_each, base);
  test<semistable_vector>(for_each, base);

  std::cout << "insert\n";
  base = test<vector>(insert);
  test<list>(insert, base);
  test<semistable_vector>(insert, base);

  std::cout << "erase_if\n";
  base = test<vector>(erase_if_);
  test<list>(erase_if_, base);
  test<semistable_vector>(erase_if_, base);

  std::cout << "sort\n";
  base = test<vector>(sort);
  test<list>(list_sort, base);
  test<semistable_vector>(sort, base);
}
