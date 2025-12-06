/* Semistable vector.
 * 
 * Copyright 2025 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef SEMISTABLE_VECTOR_HPP
#define SEMISTABLE_VECTOR_HPP

#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS)
#include <concepts>
#endif

#if defined(BOOST_NO_CXX20_HDR_RANGES)
#define SEMISTABLE_NO_CXX20_HDR_RANGES
#elif BOOST_WORKAROUND(BOOST_CLANG_VERSION, < 170100) && \
      defined(BOOST_LIBSTDCXX_VERSION)
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109647
 * https://github.com/llvm/llvm-project/issues/49620
 */
#define SEMISTABLE_NO_CXX20_HDR_RANGES
#endif

#if !defined(SEMISTABLE_NO_CXX20_HDR_RANGES)
#include <ranges>
#endif

#if defined(SEMISTABLE_ENABLE_INVARIANT_CHECKING)
#if !defined(SEMISTABLE_ASSERT)
#if !defined(NDEBUG)
#include <cassert>
#define SEMISTABLE_ASSERT assert
#else 
#error SEMISTABLE_ASSERT must be user defined when invariant checking \
is enabled in NDEBUG mode
#endif
#endif
#endif

namespace semistable {

template<typename, typename> class vector;

namespace detail {

template<typename T>
struct epoch;

template<typename T>
using epoch_pointer = std::shared_ptr<epoch<T>>;

template<typename T>
struct epoch
{
  epoch(
    T* data_ = nullptr, std::size_t index_ = 0, std::ptrdiff_t offset_ =0):
    data{data_}, index{index_}, offset{offset_} {}

  bool try_fuse(epoch& x) noexcept
  {
    if(
      (   offset <= 0 &&    x.index == index) ||
      (/* offset >  0 && */ x.index >= index && x.index <= index + offset)) {
      data = x.data;
      offset += x.offset;
      next = std::move(x.next);
      return true;
    }
    return false;
  }

  ~epoch()
  {
    /* prevents recursive destruction */

    while(next.use_count() == 1) {
      auto tmp = std::move(next);
      next = std::move(tmp->next);
    }
  }

  T*               data;
  std::size_t      index;
  std::ptrdiff_t   offset;
  epoch_pointer<T> next;
};

template<typename T>
class iterator
{
  using epoch_pointer =
    detail::epoch_pointer<typename std::remove_const<T>::type>;
  template<typename Q>
  using enable_if_consts_to_value_type_t =
    typename std::enable_if<std::is_same<const Q, T>::value>::type*;

public:
  using value_type = typename std::remove_const<T>::type;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;

#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS)
  using iterator_category = std::contiguous_iterator_tag;
#else
  using iterator_category = std::random_access_iterator_tag;
#endif

  iterator(std::size_t idx_ = 0, epoch_pointer pe_ = nullptr):
    idx{idx_}, pe{pe_} {}
  iterator(const iterator&) = default;
  iterator(iterator&&) = default;

  template<
    typename Q,
    enable_if_consts_to_value_type_t<Q> = nullptr
  >
  iterator(const iterator<Q>& x):
    idx{x.idx}, pe{x.pe} {}
      
  template<
    typename Q,
    enable_if_consts_to_value_type_t<Q> = nullptr
  >
  iterator(iterator<Q>&& x):
    idx{x.idx}, pe{std::move(x.pe)} {}

  iterator& operator=(const iterator&) = default;

  template<
    typename Q,
    enable_if_consts_to_value_type_t<Q> = nullptr
  >
  iterator& operator=(const iterator<Q>& x)
  {
    idx = x.idx;
    pe = x.pe;
    return *this;
  }

  template<
    typename Q,
    enable_if_consts_to_value_type_t<Q> = nullptr
  >
  iterator& operator=(iterator<Q>&& x)
  {
    idx = x.idx;
    pe = std::move(x.pe);
    return *this;
  }

  pointer raw() const noexcept 
  {
    update();
    return pe->data+idx;
  }

  pointer operator->() const noexcept
  {
    return raw();
  }

  reference operator*() const noexcept
  {
    return *raw();
  }

  iterator& operator++() noexcept
  {
    ++index();
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator tmp(*this);
    ++index();
    return tmp;
  }

  iterator& operator--() noexcept
  {
    --index();
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator tmp(*this);
    --index();
    return tmp;
  }

  friend difference_type
  operator-(const iterator& x, const iterator& y) noexcept
  {
    return (difference_type)(x.index() - y.index());
  }

  iterator& operator+=(difference_type n) noexcept
  {
    index() += n;
    return *this;
  }
    
  friend iterator operator+(const iterator& x, difference_type n) noexcept
  {
    x.update();
    return {x.idx + n, x.pe};
  }

  friend iterator operator+(difference_type n, const iterator& x) noexcept
  {
    x.update();
    return {n + x.idx, x.pe};
  }

  iterator& operator-=(difference_type n) noexcept
  {
    index() -= n;
    return *this;
  }
    
  friend iterator operator-(const iterator& x, difference_type n) noexcept
  {
    x.update();
    return {x.idx - n, x.pe};
  }

  reference operator[](difference_type n)const noexcept
  {
    return raw()[n];
  }

  friend bool operator==(const iterator& x, const iterator& y) noexcept
  {
    return x.index() == y.index();
  }
  
  friend bool operator!=(const iterator& x, const iterator& y) noexcept
  {
    return x.index() != y.index();
  }

  friend bool operator<(const iterator& x, const iterator& y) noexcept
  {
    return x.index() < y.index();
  }

  friend bool operator>(const iterator& x, const iterator& y) noexcept
  {
    return x.index() > y.index();
  }

  friend bool operator<=(const iterator& x, const iterator& y) noexcept
  {
    return x.index() <= y.index();
  }

  friend bool operator>=(const iterator& x, const iterator& y) noexcept
  {
    return x.index() >= y.index();
  }

private:
  template<typename> friend class iterator;
  template<typename, typename> friend class semistable::vector;

  void update() const noexcept
  {
    while(BOOST_UNLIKELY(pe->next.get() != nullptr)){
      pe = pe->next;
      auto& e = *pe;
      if(idx >= e.index) idx += e.offset;
    }
  }
  
  std::size_t& index() const noexcept
  {
    update();
    return idx;
  }

  std::size_t unsafe_index() const noexcept
  {
    return idx;
  }

  mutable std::size_t   idx;
  mutable epoch_pointer pe;
};

template<typename T>
struct type_identity { using type = T; };

template<typename T>
using type_identity_t = typename type_identity<T>::type;

#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS) && \
    !defined(SEMISTABLE_NO_CXX20_HDR_RANGES)
template<typename R, typename T>
concept container_compatible_range =
  std::ranges::input_range<R> &&
  std::convertible_to<std::ranges::range_reference_t<R>, T>;
#endif

struct access
{
  template<typename T, typename Allocator>
  static const typename semistable::vector<T, Allocator>::impl_type&
  get_impl(const semistable::vector<T, Allocator>& x)
  {
    return x.impl; 
  }

#if defined(SEMISTABLE_ENABLE_INVARIANT_CHECKING)
  template<typename T, typename Allocator>
  static bool check_invariant(const semistable::vector<T, Allocator>& x)
  {
    return x.check_invariant(); 
  }
#endif
};

#if defined(SEMISTABLE_ENABLE_INVARIANT_CHECKING)

template<typename T, typename Allocator>
struct invariant_checker
{
  ~invariant_checker() { SEMISTABLE_ASSERT(access::check_invariant(x)); }
  const semistable::vector<T, Allocator>& x;
};

template<typename T, typename Allocator>
invariant_checker<T, Allocator>
make_invariant_checker(const semistable::vector<T, Allocator>& x)
{
  return {x};
}

#define SEMISTABLE_PP_CAT_I(x, y) x##y
#define SEMISTABLE_PP_CAT(x, y) SEMISTABLE_PP_CAT_I(x, y)

#define SEMISTABLE_CHECK_INVARIANT_OF(x)             \
auto SEMISTABLE_PP_CAT(check_invariant_, __LINE__) = \
  ::semistable::detail::make_invariant_checker(x);   \
(void)SEMISTABLE_PP_CAT(check_invariant_, __LINE__)

#define SEMISTABLE_CHECK_INVARIANT SEMISTABLE_CHECK_INVARIANT_OF(*this)

#else

#define SEMISTABLE_CHECK_INVARIANT_OF(x) (void)0
#define SEMISTABLE_CHECK_INVARIANT (void)0

#endif

} /* namespace detail */

template<typename T, typename Allocator = std::allocator<T>>
class vector
{
  using impl_type = std::vector<T, Allocator>;
  using epoch_type = detail::epoch<T>;
  using epoch_pointer = detail::epoch_pointer<T>;

public:
  /* types */

  using value_type = typename impl_type::value_type;
  using allocator_type = typename impl_type::allocator_type;
  using pointer = typename impl_type::pointer;
  using const_pointer = typename impl_type::const_pointer;
  using reference = typename impl_type::reference;
  using const_reference = typename impl_type::const_reference;
  using size_type = typename impl_type::size_type;
  using difference_type = typename impl_type::difference_type;
  using iterator = detail::iterator<T>;
  using const_iterator = detail::iterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS)
  static_assert(std::contiguous_iterator<iterator>);
  static_assert(std::contiguous_iterator<const_iterator>);
#endif

  /* construct/copy/destroy */

  vector()
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  explicit vector(const Allocator& al): impl{al}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  vector(size_type n, const Allocator& al = Allocator()):
#if defined(_LIBCPP_VERSION) && BOOST_CXX_VERSION < 201402L
    /* vector(size_type, const_allocator&) only provided on C++14 and later */
    impl{n, T{}, al}
#else
    impl{n, al}
#endif
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  vector(size_type n, const T& value, const Allocator& al = Allocator()):
    impl{n, value, al}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  template<
    typename InputIterator,
    typename std::enable_if<
      std::is_convertible<
        typename std::iterator_traits<InputIterator>::iterator_category,
        std::input_iterator_tag
      >::value
    >::type* =nullptr
  >
  vector(
    InputIterator first, InputIterator last,
    const Allocator & al = Allocator()): impl{first, last, al}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS) && \
    !defined(SEMISTABLE_NO_CXX20_HDR_RANGES)
  template<
    typename FromRangeT, detail::container_compatible_range<T> R,
    typename std::enable_if<
      std::is_constructible<impl_type, FromRangeT, R&&, const Allocator>::value
    >::type* = nullptr
  >
  vector(FromRangeT, R&& rg, const Allocator& al = Allocator()): 
    impl{FromRangeT(), std::forward<R>(rg), al}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }
#endif

  vector(const vector& x): impl{x.impl}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  vector(vector&& x): vector{std::move(x), std::make_shared<epoch_type>()} {}

  vector(const vector& x, const detail::type_identity_t<Allocator>& al):
    impl{x.impl, al}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  vector(vector&& x, const detail::type_identity_t<Allocator>& al):
    vector{
      std::move(x), al,
      std::make_shared<epoch_type>(), x.size(),
      x.impl.get_allocator() == al?
        epoch_pointer{}:
        std::make_shared<epoch_type>()} {}

  vector(std::initializer_list<T> il, const Allocator& al = Allocator()):
    impl{il,al}
  {
    SEMISTABLE_CHECK_INVARIANT;
  }

  vector& operator=(const vector& x)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl = x.impl;
      return epoch_type{impl.data(), n, (difference_type)(impl.size() - n)};
    });
    return *this;
  }

  vector& operator=(vector&& x)
  {
    SEMISTABLE_CHECK_INVARIANT_OF(*this);
    SEMISTABLE_CHECK_INVARIANT_OF(x);
    auto pe_for_x = std::make_shared<epoch_type>(),
         pe_for_this = impl.get_allocator() ==x.impl.get_allocator()?
           epoch_pointer{}:
           std::make_shared<epoch_type>();
    impl = std::move(x.impl);
    if(!pe_for_this) { /* equal allocators */
      pe = std::move(x.pe);
      pe1 = std::move(x.pe1);
      pe2 = std::move(x.pe2);
      x.pe =std::move(pe_for_x);
      *x.pe = {x.impl.data()};
    }
    else{ /* unequal allocators */
      pe = std::move(pe_for_this);
      *pe = {impl.data()};
      x.new_epoch(std::move(pe_for_x), [&, this] {
        auto n = impl.size();
        return epoch_type{
          x.impl.data(), n, (std::ptrdiff_t)(x.impl.size() - n)};
      });
    }
    return *this;
  }

  vector& operator=(std::initializer_list<T> il)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl = il;
      return epoch_type{impl.data(), n, (difference_type)(impl.size() - n)};
    });
    return *this;
  }

  template<class InputIterator>
  void assign(InputIterator first, InputIterator last)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.assign(first, last);
      return epoch_type{impl.data(), n, (difference_type)(impl.size() - n)};
    });
  }

  template<
    typename R,
    typename ImplType = impl_type,
    typename std::enable_if<
      sizeof(
        std::declval<ImplType>().assign_range(std::declval<R&&>()), 0) != 0
    >::type* =nullptr
  >
  void assign_range(R&& rg)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.assign_range(std::forward<R>(rg));
      return epoch_type{impl.data(), n, (difference_type)(impl.size() - n)};
    });
  }

  void assign(size_type n, const T& value)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto m = impl.size();
      impl.assign(n, value);
      return epoch_type{impl.data(), m, (difference_type)(n - m)};
    });
  }

  void assign(std::initializer_list<T> il)
  {
    assign(il.begin(), il.end()); 
  }

  allocator_type get_allocator() const noexcept
  {
    return impl.get_allocator();
  }

  /* iterators */

  iterator               begin() noexcept { return {0, pe}; }
  const_iterator         begin() const noexcept { return {0, pe}; }
  iterator               end() noexcept { return {impl.size(), pe}; }
  const_iterator         end() const noexcept { return {impl.size(), pe}; }
  reverse_iterator       rbegin() noexcept { return reverse_iterator{end()}; }
  const_reverse_iterator rbegin() const noexcept 
                         { return const_reverse_iterator{end()};}
  reverse_iterator       rend() noexcept { return reverse_iterator{begin()}; }
  const_reverse_iterator rend() const noexcept 
                         { return const_reverse_iterator{begin()}; }

  const_iterator         cbegin() const noexcept { return begin(); }
  const_iterator         cend() const noexcept { return end(); }
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  const_reverse_iterator crend() const noexcept { return rend(); }

  /* capacity */

  bool      empty() const noexcept { return impl.empty(); }
  size_type size() const noexcept { return impl.size(); }
  size_type max_size() const noexcept { return impl.max_size(); }
  size_type capacity() const noexcept { return impl.capacity(); }

  void resize(size_type n)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto m = impl.size();
      impl.resize(n);
      return epoch_type{impl.data(), m, (difference_type)(n - m)};
    });
  }

  void resize(size_type n, const T& value)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto m = impl.size();
      impl.resize(n, value);
      return epoch_type{impl.data(), m, (difference_type)(n - m)};
    });
  }

  void reserve(size_type n)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      impl.reserve(n);
      return epoch_type{impl.data(), pe->index};
    });
  }

  void shrink_to_fit()
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      impl.shrink_to_fit();
      return epoch_type{impl.data(), pe->index};
    });
  }

  /* element access */

  reference       operator[](size_type n) { return impl[n]; }
  const_reference operator[](size_type n) const { return impl[n]; }
  reference       at(size_type n) { return impl.at(n); }
  const_reference at(size_type n) const { return impl.at(n); }
  reference       front() { return impl.front(); }
  const_reference front() const { return impl.front(); }
  reference       back() { return impl.back(); }
  const_reference back() const { return impl.back(); }

  /* data access */

  T*       data() noexcept { return impl.data(); }
  const T* data() const noexcept { return impl.data(); }

  /* modifiers */

  template<typename... Args>
  reference emplace_back(Args&&... args)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.emplace_back(std::forward<Args>(args)...);
      return epoch_type{impl.data(), n, 1};
    });
    return impl.back();
  }

  void push_back(const T& x)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.push_back(x);
      return epoch_type{impl.data(), n, 1};
    });
  }

  void push_back(T&& x)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.push_back(std::move(x));
      return epoch_type{impl.data(), n, 1};
    });
  }

  template<
    typename R,
    typename ImplType = impl_type,
    typename std::enable_if<
      sizeof(
        std::declval<ImplType>().append_range(std::declval<R&&>()), 0) != 0
    >::type* =nullptr
  >
  void append_range(R&& rg)
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.append_range(std::forward<R>(rg));
      return epoch_type{
        impl.data(), n, (difference_type)(impl.size() - n)};
    });
  }

  void pop_back()
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([&, this] {
      auto n = impl.size();
      impl.pop_back();
      return epoch_type{impl.data(), n, -1};
    });
  }

  template<typename... Args>
  iterator emplace(const_iterator pos, Args&&... args)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      impl.emplace(impl.begin() + index, std::forward<Args>(args)...);
      return epoch_type{impl.data(), index, 1};
    });
    return {index, pe};
  }

  iterator insert(const_iterator pos, const T& x)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      impl.insert(impl.begin() + index, x);
      return epoch_type{impl.data(), index, 1};
    });
    return {index, pe};
  }

  iterator insert(const_iterator pos, T&& x)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      impl.insert(impl.begin() + index, std::move(x));
      return epoch_type{impl.data(), index, 1};
    });
    return {index, pe};
  }

  iterator insert(const_iterator pos, size_type n, const T& x)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      impl.insert(impl.begin() + index, n, x);
      return epoch_type{impl.data(), index, (difference_type)n};
    });
    return {index, pe};
  }

  template<class InputIterator>
  iterator insert(const_iterator pos,InputIterator first, InputIterator last)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      auto m = impl.size();
      impl.insert(impl.begin() + index, first, last);
      return epoch_type{
        impl.data(), index, (difference_type)(impl.size() - m)};
    });
    return {index, pe};
  }

  template<
    typename R,
    typename ImplType = impl_type,
    typename std::enable_if<
      sizeof(
        std::declval<ImplType>().insert_range(
          std::declval<const_iterator>(), std::declval<R&&>()), 0) != 0
    >::type* =nullptr
  >
  iterator insert_range(const_iterator pos, R&& rg)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      auto m = impl.size();
      impl.insert_range(impl.begin() + index, std::forward<R>(rg));
      return epoch_type{
        impl.data(), index, (difference_type)(impl.size() - m)};
    });
    return {index, pe};
  }

  iterator insert(const_iterator pos, std::initializer_list<T> il)
  {
    return insert(pos, il.begin(), il.end());
  }

  iterator erase(const_iterator pos)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto index = pos.unsafe_index();
    new_epoch([&, this] {
      impl.erase(impl.begin() + index);
      return epoch_type{impl.data(), index + 1, -1};
    });
    return {index, pe};
  }

  iterator erase(const_iterator first, const_iterator last)
  {
    SEMISTABLE_CHECK_INVARIANT;
    auto findex = first.unsafe_index(), 
         lindex = last.unsafe_index();
    new_epoch([&, this] {
      impl.erase(impl.begin() + findex, impl.begin() + lindex);
      return epoch_type{
        impl.data(), findex + 1, (difference_type)(findex - lindex)};
    });
    return {findex, pe};
  }

  void swap(vector& x)
#if !defined(SEMISTABLE_ENABLE_INVARIANT_CHECKING)
    noexcept(noexcept(
      std::declval<impl_type&>().swap(std::declval<impl_type&>())))
#endif
  {
    SEMISTABLE_CHECK_INVARIANT_OF(*this);
    SEMISTABLE_CHECK_INVARIANT_OF(x);
    impl.swap(x.impl);
    pe.swap(x.pe);
    pe1.swap(x.pe1);
    pe2.swap(x.pe2);
  }

  void clear()
  {
    SEMISTABLE_CHECK_INVARIANT;
    new_epoch([this] {
      auto n = impl.size();
      impl.clear();
      return epoch_type{impl.data(), n, -(std::ptrdiff_t)n};
    });
  }

private:
  friend struct detail::access;

  vector(vector&& x, epoch_pointer pe_for_x)
#if !defined(SEMISTABLE_ENABLE_INVARIANT_CHECKING)
    noexcept
#endif
    :    
    impl{std::move(x.impl)},
    pe{std::move(x.pe)}, pe1{std::move(x.pe1)}, pe2{std::move(x.pe2)}
  {
    SEMISTABLE_CHECK_INVARIANT_OF(*this);
    SEMISTABLE_CHECK_INVARIANT_OF(x);
    x.pe = std::move(pe_for_x);
    *x.pe = {x.impl.data()};
  }

  vector(
    vector&& x, const Allocator& al,
    epoch_pointer pe_for_x, size_type x_size, epoch_pointer pe_for_this):
    impl{std::move(x.impl), al}, pe{}, pe1{}, pe2{}
  {
    // TODO: make safe against exceptions in impl construction
    if(!pe_for_this) { /* equal allocators */
      pe = std::move(x.pe);
      pe1 = std::move(x.pe1);
      pe2 = std::move(x.pe2);
      x.pe = std::move(pe_for_x);
      *x.pe = {x.impl.data()};
    }
    else{ /* unequal allocators */
      pe = std::move(pe_for_this);
      *pe = {impl.data()};
      x.new_epoch(std::move(pe_for_x), [&, this] {
        return epoch_type{
          x.impl.data(), x_size, (std::ptrdiff_t)(x_size - x.impl.size())};
      });
    }
    SEMISTABLE_CHECK_INVARIANT_OF(*this);
    SEMISTABLE_CHECK_INVARIANT_OF(x);
  }

  template<typename F>
  void new_epoch(F f)
  {
    new_epoch(make_epoch_pointer(), f);
  }

  template<typename F>
  void new_epoch(epoch_pointer&& next, F f) noexcept(noexcept(f()))
  {
    *next = f();
    pe->next = next;
    pe2 = std::move(pe1);
    pe1 = std::move(pe);
    pe = std::move(next);
  }

  epoch_pointer make_epoch_pointer()
  {
    long pe2c, pe1c;

    if((pe2c = pe2.use_count()) == 1) {
      /* pe2 available for reuse */
      return std::move(pe2);
    }
    else if((pe1c = pe1.use_count()) == 1) {
      /* pe2 empty, pe1 available for reuse */
      return std::move(pe1);
    }
    else if(pe2c == 2 && pe1c == 2 && pe2->try_fuse(*pe1)) {
      /*  no iterator at *pe2 or *pe1 and we can fuse *pe1 into *pe2 */
      auto tmp = std::move(pe1);
      pe1 = std::move(pe2);
      return std::move(tmp);
    }
    return std::make_shared<epoch_type>();
  }

#if defined(SEMISTABLE_ENABLE_INVARIANT_CHECKING)
  bool check_invariant() const noexcept
  {
    return
      pe && pe->data == impl.data() && !pe->next &&
      (!pe1 || pe1->next == pe) &&
      (!pe2 || (pe1 && pe2->next == pe1));
  }
#endif
  
  impl_type     impl;
  epoch_pointer pe = std::make_shared<epoch_type>(epoch_type{impl.data()}),
                pe1, pe2; /* pointers to two epochs prior */
};

#if !defined(BOOST_NO_CXX17_DEDUCTION_GUIDES)
template<
  typename InputIterator, 
  typename Allocator = std::allocator<
    typename std::iterator_traits<InputIterator>::value_type>
>
vector(InputIterator, InputIterator, Allocator = Allocator())
  -> vector<
    typename std::iterator_traits<InputIterator>::value_type, Allocator>;

#if !defined(BOOST_NO_CXX20_HDR_CONCEPTS) && \
    !defined(SEMISTABLE_NO_CXX20_HDR_RANGES)
template<
  typename FromRangeT,
  std::ranges::input_range R,
  typename Allocator = std::allocator<std::ranges::range_value_t<R>>,
  typename std::enable_if<
    std::is_constructible<
      std::vector<std::ranges::range_value_t<R>, Allocator>,
      FromRangeT, R&&, const Allocator>::value
  >::type* = nullptr
>
vector(FromRangeT, R&&, Allocator = Allocator())
  -> vector<std::ranges::range_value_t<R>, Allocator>;
#endif
#endif

template<typename T, typename Allocator>
bool operator==(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
  return detail::access::get_impl(x) == detail::access::get_impl(y);
}

template<typename T, typename Allocator>
bool operator!=(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
  return detail::access::get_impl(x) != detail::access::get_impl(y);
}

template<typename T, typename Allocator>
bool operator<(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
  return detail::access::get_impl(x) < detail::access::get_impl(y);
}

template<typename T, typename Allocator>
bool operator<=(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
  return detail::access::get_impl(x) <= detail::access::get_impl(y);
}

template<typename T, typename Allocator>
bool operator>(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
  return detail::access::get_impl(x) > detail::access::get_impl(y);
}

template<typename T, typename Allocator>
bool operator>=(const vector<T, Allocator>& x, const vector<T, Allocator>& y)
{
  return detail::access::get_impl(x) >= detail::access::get_impl(y);
}

template<typename T, typename Allocator>
void swap(vector<T, Allocator>& x, vector<T, Allocator>& y)
  noexcept(noexcept(x.swap(y)))
{
  x.swap(y);
}

template<typename T, typename Allocator, typename Predicate>
typename vector<T, Allocator>::size_type
erase_if(vector<T, Allocator>& x, Predicate pred)
{
  typename vector<T, Allocator>::size_type res = 0;
  for(auto first = x.begin(), last = x.end(); first != last;) {
    if(pred(*first)) {
      x.erase(first++);
      ++res;
    }
    else ++first;
  }
  return res;
}

template<typename T, typename Allocator, typename U = T>
typename vector<T, Allocator>::size_type
erase(vector<T, Allocator>& x, const U& value)
{
  using value_type = typename vector<T, Allocator>::value_type;
  return erase_if(x, [&](const value_type& v) { return v == value; });
}

} /* namespace semistable */

#endif
