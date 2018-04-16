/*
  Copyright (C) 2014 Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
  * Neither the name of Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INDEXED_VALUE_H
#define INDEXED_VALUE_H

#include <cstddef>
#include <iostream>
#include <iterator>
#include <utility>
#include <atomic>
#include <cassert>

//Forward Declarations
void CreateDataSet(size_t n);
void CheckIsSorted(size_t n);
void Test(size_t n);

//! Number of extant keys
static std::atomic<int> KeyCount;

//! Highest index in array to be sorted.
static unsigned HighValidIndex;

//! A key to be sorted, with lots of checking.
class Key {
  //! Value used by comparator
  int value;
  //! Original position or special vlaue (Empty or Dead)
  int index;
  //! Special value used to mark constructed object without a comparable
  //! value.
  static const int Empty = -1;
  //! Special value used to mark destroyed objects.
  static const int Dead = -2;

public:
  Key()
  {
    ++KeyCount;
    index = Empty;
  }
  Key(const Key& k)
    : value(k.value)
    , index(k.index)
  {
    assert(k.isLive());
    ++KeyCount;
  }
  Key(Key&& k)
    : value(k.value)
    , index(k.index)
  {
    assert(k.isConstructed());
    k.index = Empty;
    ++KeyCount;
  }
  ~Key()
  {
    assert(isConstructed());
    index = Dead;
    --KeyCount;
  }
  void operator=(Key&& k)
  {
    assert(k.isConstructed());
    assert(isConstructed());
    value   = k.value;
    index   = k.index;
    k.index = Empty;
  }
  void operator=(const Key& k)
  {
    assert(k.isLive());
    assert(isConstructed());
    value = k.value;
    index = k.index;
  }
  bool isConstructed() const
  {
    return isLive() || index == Empty;
  }
  bool isLive() const
  {
    return unsigned(index) <= HighValidIndex;
  }
  friend class KeyCompare;
  friend void CreateDataSet(size_t n);
  friend void CheckIsSorted(size_t n);
};

class KeyCompare {
  enum statusType {
    //! Special value used to defined object.
    Live = 0xabcd,
    //! Special value used to mark default-constructed object.
    Empty = -1,
    //! Special value used to mark destroyed objects.
    Dead = -2
  } status;
  KeyCompare(statusType s)
    : status(s)
  {
  }

public:
  KeyCompare()
  {
    status = Empty;
  }
  ~KeyCompare()
  {
    status = Dead;
  }
  bool operator()(const Key& j, const Key& k) const
  {
    assert(status == Live);
    assert(j.isLive());
    assert(k.isLive());
    return j.value < k.value;
  }
  friend void Test(size_t n);
};

//! Iterator that points to a key, with some checking.
class Iterator {
public:
  typedef std::random_access_iterator_tag iterator_category;
  typedef Key                        value_type;
  typedef value_type&                reference;
  typedef std::ptrdiff_t             difference_type;
  typedef Key*                       pointer;

private:
  Key* my_ptr;
  //! [my_begin,my_end) is the legal range for my_ptr if this Iterator has
  //! defined value.
  Key *       my_begin, *my_end;
  static Key* poison(long val)
  {
    return (Key*)val;
  }
  bool isDefined() const
  {
    assert(my_begin <= my_ptr);
    assert(my_ptr <= my_end);
    return true;
  }

public:
  Iterator(Key* begin, Key* end, size_t offset)
    : my_ptr(begin + offset)
    , my_begin(begin)
    , my_end(end)
  {
  }
  //! Construct undefined iterator
  Iterator()
    : my_ptr(0)
    , my_begin(0)
    , my_end(poison(-1))
  {
  }

  ~Iterator()
  {
    my_begin = poison(-2);
    my_end   = poison(-3);
  }

  Iterator& operator+=(std::ptrdiff_t n)
  {
    assert(isDefined());
    assert(my_begin <= my_ptr + n);
    assert(my_ptr + n <= my_end);
    my_ptr += n;
    return *this;
  }

  Iterator& operator++()
  {
    return operator+=(1);
  }

  Iterator& operator--()
  {
    return operator+=(-1);
  }

  friend Iterator operator+(const Iterator& i, const std::ptrdiff_t n)
  {
    Iterator j(i);
    return j += n;
  }
  friend Iterator operator-(const Iterator& i, const std::ptrdiff_t n)
  {
    Iterator j(i);
    return j += -n;
  }
  friend std::ptrdiff_t operator-(const Iterator& i, const Iterator& j)
  {
    assert(i.isDefined());
    assert(j.isDefined());
    return i.my_ptr - j.my_ptr;
  }
  friend bool operator==(const Iterator& i, const Iterator& j)
  {
    assert(i.isDefined());
    assert(j.isDefined());
    return i.my_ptr == j.my_ptr;
  }
  friend bool operator!=(const Iterator& i, const Iterator& j)
  {
    assert(i.isDefined());
    assert(j.isDefined());
    return i.my_ptr != j.my_ptr;
  }
  friend bool operator<(const Iterator& i, const Iterator& j)
  {
    assert(i.isDefined());
    assert(j.isDefined());
    return i.my_ptr < j.my_ptr;
  }
  Key& operator*()
  {
    assert(isDefined());
    return *my_ptr;
  }
};

#if 0
template <typename I, typename V>
bool verify(Iterator<I, V> const begin, Iterator<I, V> const end)
{
  using value_type =
      typename std::iterator_traits<Iterator<I, V>>::value_type;

  auto strict_cmp = [](value_type const& lhs, value_type const& rhs) {
    auto cmp = KeyCompare<I, V>(KeyCompare<I, V>::Live);
    return (!cmp(lhs, rhs) || (!cmp(rhs, lhs) && (rhs.index > rhs.index)));
  };

  return parallel_verify(begin, end, strict_cmp);
}

template <typename I, typename V>
void rand(Iterator<I, V> const begin, Iterator<I, V> const end)
{
  using value_type =
      typename std::iterator_traits<Iterator<I, V>>::value_type;

  auto const n = static_cast<size_t>(std::distance(begin, end));

  auto gen = [n](std::size_t idx) {
    value_type val;
    val.index = idx;
    val.value = rand() % (2 * n);
    return val;
  };

  parallel_gen(begin, end, gen);
}
#endif

#endif
