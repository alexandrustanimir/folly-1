/*
 * Copyright 2012 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @author Mark Rabkin (mrabkin@fb.com)
// @author Andrei Alexandrescu (andrei.alexandrescu@fb.com)

#ifndef FOLLY_RANGE_H_
#define FOLLY_RANGE_H_

#include "folly/FBString.h"
#include <glog/logging.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <boost/operators.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <bits/c++config.h>
#include "folly/Traits.h"

namespace folly {

template <class T> class Range;

/**
Finds the first occurrence of needle in haystack. The algorithm is on
average faster than O(haystack.size() * needle.size()) but not as fast
as Boyer-Moore. On the upside, it does not do any upfront
preprocessing and does not allocate memory.
 */
template <class T>
inline size_t qfind(const Range<T> & haystack,
                    const Range<T> & needle);

/**
Finds the first occurrence of needle in haystack. The result is the
offset reported to the beginning of haystack, or string::npos if
needle wasn't found.
 */
template <class T>
size_t qfind(const Range<T> & haystack,
             const typename Range<T>::value_type& needle);

/**
 * Small internal helper - returns the value just before an iterator.
 */
namespace detail {

/**
 * For random-access iterators, the value before is simply i[-1].
 */
template <class Iter>
typename boost::enable_if_c<
  boost::is_same<typename std::iterator_traits<Iter>::iterator_category,
                 std::random_access_iterator_tag>::value,
  typename std::iterator_traits<Iter>::reference>::type
value_before(Iter i) {
  return i[-1];
}

/**
 * For all other iterators, we need to use the decrement operator.
 */
template <class Iter>
typename boost::enable_if_c<
  !boost::is_same<typename std::iterator_traits<Iter>::iterator_category,
                  std::random_access_iterator_tag>::value,
  typename std::iterator_traits<Iter>::reference>::type
value_before(Iter i) {
  return *--i;
}

} // namespace detail

/**
 * Range abstraction keeping a pair of iterators. We couldn't use
 * boost's similar range abstraction because we need an API identical
 * with the former StringPiece class, which is used by a lot of other
 * code. This abstraction does fulfill the needs of boost's
 * range-oriented algorithms though.
 *
 * (Keep memory lifetime in mind when using this class, since it
 * doesn't manage the data it refers to - just like an iterator
 * wouldn't.)
 */
template <class Iter>
class Range : private boost::totally_ordered<Range<Iter> > {
public:
  typedef std::size_t size_type;
  typedef Iter iterator;
  typedef Iter const_iterator;
  typedef typename boost::remove_reference<
    typename std::iterator_traits<Iter>::reference>::type
  value_type;
  typedef typename std::iterator_traits<Iter>::reference reference;
  typedef std::char_traits<value_type> traits_type;

  static const size_type npos = -1;

  // Works for all iterators
  Range() : b_(), e_() {
  }

private:
  static bool reachable(Iter b, Iter e, std::forward_iterator_tag) {
    for (; b != e; ++b) {
      LOG_EVERY_N(INFO, 100000) << __FILE__ ":" << __LINE__
                                << " running reachability test ("
                                << google::COUNTER << " iterations)...";
    }
    return true;
  }

  static bool reachable(Iter b, Iter e, std::random_access_iterator_tag) {
    return b <= e;
  }

public:
  // Works for all iterators
  Range(Iter start, Iter end)
      : b_(start), e_(end) {
    assert(reachable(b_, e_,
                     typename std::iterator_traits<Iter>::iterator_category()));
  }

  // Works only for random-access iterators
  Range(Iter start, size_t size)
      : b_(start), e_(start + size) { }

  // Works only for Range<const char*>
  /* implicit */ Range(Iter str)
      : b_(str), e_(b_ + strlen(str)) {}
  // Works only for Range<const char*>
  /* implicit */ Range(const std::string& str)
      : b_(str.data()), e_(b_ + str.size()) {}
  // Works only for Range<const char*>
  Range(const std::string& str, std::string::size_type startFrom) {
    CHECK_LE(startFrom, str.size());
    b_ = str.data() + startFrom;
    e_ = str.data() + str.size();
  }
  // Works only for Range<const char*>
  Range(const std::string& str,
        std::string::size_type startFrom,
        std::string::size_type size) {
    CHECK_LE(startFrom + size, str.size());
    b_ = str.data() + startFrom;
    e_ = b_ + size;
  }
  // Works only for Range<const char*>
  /* implicit */ Range(const fbstring& str)
    : b_(str.data()), e_(b_ + str.size()) { }
  // Works only for Range<const char*>
  Range(const fbstring& str, fbstring::size_type startFrom) {
    CHECK_LE(startFrom, str.size());
    b_ = str.data() + startFrom;
    e_ = str.data() + str.size();
  }
  // Works only for Range<const char*>
  Range(const fbstring& str, fbstring::size_type startFrom,
        fbstring::size_type size) {
    CHECK_LE(startFrom + size, str.size());
    b_ = str.data() + startFrom;
    e_ = b_ + size;
  }

  void clear() {
    b_ = Iter();
    e_ = Iter();
  }

  void assign(Iter start, Iter end) {
    b_ = start;
    e_ = end;
  }

  void reset(Iter start, size_type size) {
    b_ = start;
    e_ = start + size;
  }

  // Works only for Range<const char*>
  void reset(const std::string& str) {
    reset(str.data(), str.size());
  }

  size_type size() const {
    assert(b_ <= e_);
    return e_ - b_;
  }
  size_type walk_size() const {
    assert(b_ <= e_);
    return std::distance(b_, e_);
  }
  bool empty() const { return b_ == e_; }
  Iter data() const { return b_; }
  Iter start() const { return b_; }
  Iter begin() const { return b_; }
  Iter end() const { return e_; }
  Iter cbegin() const { return b_; }
  Iter cend() const { return e_; }
  value_type& front() {
    assert(b_ < e_);
    return *b_;
  }
  value_type& back() {
    assert(b_ < e_);
    return detail::value_before(e_);
  }
  const value_type& front() const {
    assert(b_ < e_);
    return *b_;
  }
  const value_type& back() const {
    assert(b_ < e_);
    return detail::value_before(e_);
  }
  // Works only for Range<const char*>
  std::string str() const { return std::string(b_, size()); }
  std::string toString() const { return str(); }
  // Works only for Range<const char*>
  fbstring fbstr() const { return fbstring(b_, size()); }
  fbstring toFbstring() const { return fbstr(); }

  // Works only for Range<const char*>
  int compare(const Range& o) const {
    const size_type tsize = this->size();
    const size_type osize = o.size();
    const size_type msize = std::min(tsize, osize);
    int r = traits_type::compare(data(), o.data(), msize);
    if (r == 0) r = tsize - osize;
    return r;
  }

  value_type& operator[](size_t i) {
    CHECK_GT(size(), i);
    return b_[i];
  }

  const value_type& operator[](size_t i) const {
    CHECK_GT(size(), i);
    return b_[i];
  }

  value_type& at(size_t i) {
    if (i >= size()) throw std::out_of_range("index out of range");
    return b_[i];
  }

  const value_type& at(size_t i) const {
    if (i >= size()) throw std::out_of_range("index out of range");
    return b_[i];
  }

  // Works only for Range<const char*>
  uint32_t hash() const {
    // Taken from fbi/nstring.h:
    //    Quick and dirty bernstein hash...fine for short ascii strings
    uint32_t hash = 5381;
    for (size_t ix = 0; ix < size(); ix++) {
      hash = ((hash << 5) + hash) + b_[ix];
    }
    return hash;
  }

  void advance(size_type n) {
    CHECK_LE(n, size());
    b_ += n;
  }

  void subtract(size_type n) {
    CHECK_LE(n, size());
    e_ -= n;
  }

  void pop_front() {
    assert(b_ < e_);
    ++b_;
  }

  void pop_back() {
    assert(b_ < e_);
    --e_;
  }

  Range subpiece(size_type first,
                 size_type length = std::string::npos) const {
    CHECK_LE(first, size());
    return Range(b_ + first,
                 std::min<std::string::size_type>(length, size() - first));
  }

  // string work-alike functions
  size_type find(Range str) const {
    return qfind(*this, str);
  }

  size_type find(Range str, size_t pos) const {
    if (pos > size()) return std::string::npos;
    size_t ret = qfind(subpiece(pos), str);
    return ret == npos ? ret : ret + pos;
  }

  size_type find(Iter s, size_t pos, size_t n) const {
    if (pos > size()) return std::string::npos;
    size_t ret = qfind(pos ? subpiece(pos) : *this, Range(s, n));
    return ret == npos ? ret : ret + pos;
  }

  size_type find(const Iter s) const {
    return qfind(*this, Range(s));
  }

  size_type find(const Iter s, size_t pos) const {
    if (pos > size()) return std::string::npos;
    size_type ret = qfind(subpiece(pos), Range(s));
    return ret == npos ? ret : ret + pos;
  }

  size_type find(value_type c) const {
    return qfind(*this, c);
  }

  size_type find(value_type c, size_t pos) const {
    if (pos > size()) return std::string::npos;
    size_type ret = qfind(subpiece(pos), c);
    return ret == npos ? ret : ret + pos;
  }

  void swap(Range& rhs) {
    std::swap(b_, rhs.b_);
    std::swap(e_, rhs.e_);
  }

private:
  Iter b_, e_;
};

template <class Iter>
const typename Range<Iter>::size_type Range<Iter>::npos;

template <class T>
void swap(Range<T>& lhs, Range<T>& rhs) {
  lhs.swap(rhs);
}

/**
 * Create a range from two iterators, with type deduction.
 */
template <class Iter>
Range<Iter> makeRange(Iter first, Iter last) {
  return Range<Iter>(first, last);
}

typedef Range<const char*> StringPiece;

std::ostream& operator<<(std::ostream& os, const StringPiece& piece);

/**
 * Templated comparison operators
 */

template <class T>
inline bool operator==(const Range<T>& lhs, const Range<T>& rhs) {
  return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template <class T>
inline bool operator<(const Range<T>& lhs, const Range<T>& rhs) {
  return lhs.compare(rhs) < 0;
}

/**
 * Specializations of comparison operators for StringPiece
 */

namespace detail {

template <class A, class B>
struct ComparableAsStringPiece {
  enum {
    value =
    (boost::is_convertible<A, StringPiece>::value
     && boost::is_same<B, StringPiece>::value)
    ||
    (boost::is_convertible<B, StringPiece>::value
     && boost::is_same<A, StringPiece>::value)
  };
};

} // namespace detail

/**
 * operator== through conversion for Range<const char*>
 */
template <class T, class U>
typename
boost::enable_if_c<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator==(const T& lhs, const U& rhs) {
  return StringPiece(lhs) == StringPiece(rhs);
}

/**
 * operator< through conversion for Range<const char*>
 */
template <class T, class U>
typename
boost::enable_if_c<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator<(const T& lhs, const U& rhs) {
  return StringPiece(lhs) < StringPiece(rhs);
}

/**
 * operator> through conversion for Range<const char*>
 */
template <class T, class U>
typename
boost::enable_if_c<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator>(const T& lhs, const U& rhs) {
  return StringPiece(lhs) > StringPiece(rhs);
}

/**
 * operator< through conversion for Range<const char*>
 */
template <class T, class U>
typename
boost::enable_if_c<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator<=(const T& lhs, const U& rhs) {
  return StringPiece(lhs) <= StringPiece(rhs);
}

/**
 * operator> through conversion for Range<const char*>
 */
template <class T, class U>
typename
boost::enable_if_c<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator>=(const T& lhs, const U& rhs) {
  return StringPiece(lhs) >= StringPiece(rhs);
}

struct StringPieceHash {
  std::size_t operator()(const StringPiece& str) const {
    return static_cast<std::size_t>(str.hash());
  }
};

/**
 * Finds substrings faster than brute force by borrowing from Boyer-Moore
 */
template <class T, class Comp>
size_t qfind(const Range<T>& haystack,
             const Range<T>& needle,
             Comp eq) {
  // Don't use std::search, use a Boyer-Moore-like trick by comparing
  // the last characters first
  auto const nsize = needle.size();
  if (haystack.size() < nsize) {
    return std::string::npos;
  }
  if (!nsize) return 0;
  auto const nsize_1 = nsize - 1;
  auto const lastNeedle = needle[nsize_1];

  // Boyer-Moore skip value for the last char in the needle. Zero is
  // not a valid value; skip will be computed the first time it's
  // needed.
  std::string::size_type skip = 0;

  auto i = haystack.begin();
  auto iEnd = haystack.end() - nsize_1;

  while (i < iEnd) {
    // Boyer-Moore: match the last element in the needle
    while (!eq(i[nsize_1], lastNeedle)) {
      if (++i == iEnd) {
        // not found
        return std::string::npos;
      }
    }
    // Here we know that the last char matches
    // Continue in pedestrian mode
    for (size_t j = 0; ; ) {
      assert(j < nsize);
      if (!eq(i[j], needle[j])) {
        // Not found, we can skip
        // Compute the skip value lazily
        if (skip == 0) {
          skip = 1;
          while (skip <= nsize_1 && !eq(needle[nsize_1 - skip], lastNeedle)) {
            ++skip;
          }
        }
        i += skip;
        break;
      }
      // Check if done searching
      if (++j == nsize) {
        // Yay
        return i - haystack.begin();
      }
    }
  }
  return std::string::npos;
}

struct AsciiCaseSensitive {
  bool operator()(char lhs, char rhs) const {
    return lhs == rhs;
  }
};

struct AsciiCaseInsensitive {
  bool operator()(char lhs, char rhs) const {
    return toupper(lhs) == toupper(rhs);
  }
};

extern const AsciiCaseSensitive asciiCaseSensitive;
extern const AsciiCaseInsensitive asciiCaseInsensitive;

template <class T>
size_t qfind(const Range<T>& haystack,
             const Range<T>& needle) {
  return qfind(haystack, needle, asciiCaseSensitive);
}

template <class T>
size_t qfind(const Range<T>& haystack,
             const typename Range<T>::value_type& needle) {
  return qfind(haystack, makeRange(&needle, &needle + 1));
}

}  // !namespace folly

FOLLY_ASSUME_FBVECTOR_COMPATIBLE_1(folly::Range);

#endif // FOLLY_RANGE_H_
