/**\file*********************************************************************
 *                                                                     \brief
 *  Algorithms library [25 lib.algorithms]
 *
 ****************************************************************************
 */
#ifndef NTL__STLX_ALGORITHM
#define NTL__STLX_ALGORITHM
#pragma once

#ifndef NTL__STLX_ITERATOR
#include "iterator.hxx"
#endif
#ifndef NTL__STLX_UTILITY
#include "utility.hxx"
#endif
#ifndef NTL__STLX_INITLIST
#include "initializer_list.hxx"
#endif

#include "cstring.hxx"
#include "functional.hxx"

namespace std
{
///\name 25.1, non-modifying sequence operations:
template <class InputIterator, class Predicate>
inline bool all_of(InputIterator first, InputIterator last, Predicate pred);

template <class InputIterator, class Predicate>
inline bool any_of(InputIterator first, InputIterator last, Predicate pred);

template <class InputIterator, class Predicate>
inline bool none_of(InputIterator first, InputIterator last, Predicate pred);


template<class InputIterator, class Function>
__forceinline
Function
  for_each(InputIterator first, InputIterator last, Function f)
{
  while ( first != last ) f(*first++);
  return f;
}


template<class InputIterator, class T>
__forceinline
InputIterator
  find(InputIterator first, InputIterator last, const T& value)
{
  while ( first != last && !(*first == value) ) ++first;
  return first;
}


template<class InputIterator, class Predicate>
__forceinline
InputIterator
  find_if(InputIterator first, InputIterator last, Predicate pred)
{
  while ( first != last && !(pred(*first) != false) ) ++first;
  return first;
}

template<class InputIterator, class Predicate>
InputIterator find_if_not(InputIterator first, InputIterator last,
                          Predicate pred);
template<class InputIterator, class Predicate>
InputIterator find_if_not(InputIterator first, InputIterator last,
                          Predicate pred);

template<class ForwardIterator1, class ForwardIterator2>
__forceinline
ForwardIterator1
  find_end(ForwardIterator1 first1, ForwardIterator1 last1,
           ForwardIterator2 first2, ForwardIterator2 last2);


template<class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
__forceinline
ForwardIterator1
  find_end(ForwardIterator1 first1, ForwardIterator1 last1,
           ForwardIterator2 first2, ForwardIterator2 last2,
           BinaryPredicate pred);


template<class ForwardIterator1, class ForwardIterator2>
__forceinline
ForwardIterator1
  find_first_of(ForwardIterator1 first1, ForwardIterator1 last1,
                ForwardIterator2 first2, ForwardIterator2 last2)
{
  for ( ; first1 != last1; ++first1 )
    for ( ForwardIterator2 j = first2; j != last2; ++ j )
      if ( *first1 == *j ) return first1;
  return first1;  // here first1 == last1
}


template<class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
__forceinline
ForwardIterator1
  find_first_of(ForwardIterator1 first1, ForwardIterator1 last1,
                ForwardIterator2 first2, ForwardIterator2 last2,
                BinaryPredicate pred)
{
  for ( ; first1 != last1; ++first1 )
    for ( ForwardIterator2 j = first2; j != last2; ++ j )
      if ( pred(*first1, *j) != false ) return first1;
  return first1;  // here first1 == last1
}


template<class ForwardIterator>
__forceinline
ForwardIterator
  adjacent_find(ForwardIterator first, ForwardIterator last)
{
  // the code is a bit messy to gain Named Return Value optimization
  if ( first == last )  return first;
  for ( ; ; )
  {
    ForwardIterator i = first;
    if ( ++i == last )  return ++first;
    if ( *first == *i ) return first;
    first = i;
  }
}


template<class ForwardIterator, class BinaryPredicate>
__forceinline
ForwardIterator
  adjacent_find(ForwardIterator first, ForwardIterator last,
                BinaryPredicate pred)
{
  if ( first == last )  return first;
  for ( ; ; )
  {
    ForwardIterator i = first;
    if ( ++i == last )  return ++first;
    if ( pred(*first, *i) != false ) return first;
    first = i;
  }
}


template<class InputIterator, class T>
__forceinline
typename iterator_traits<InputIterator>::difference_type
  count(InputIterator first, InputIterator last, const T& value)
{
  typename iterator_traits<InputIterator>::difference_type count = 0;
  for ( ; first != last; ++first )
    if ( *first == value ) ++count;
  return count;
}


template<class InputIterator, class Predicate>
__forceinline
typename iterator_traits<InputIterator>::difference_type
  count_if(InputIterator first, InputIterator last, Predicate pred)
{
  typename iterator_traits<InputIterator>::difference_type count = 0;
  for ( ; first != last; ++first )
    if ( pred(*first) != false ) ++count;
  return count;
}


template<class InputIterator1, class InputIterator2>
__forceinline
pair<InputIterator1, InputIterator2>
  mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
{
  while ( first1 != last1 && *first1 == *first2 )
  {
    ++first1;
    ++first2;
  }
  return pair<InputIterator1, InputIterator2>(first1, first2);
}


template
<class InputIterator1, class InputIterator2, class BinaryPredicate>
__forceinline
pair<InputIterator1, InputIterator2>
  mismatch(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, BinaryPredicate pred)
{
  while ( first1 != last1 && pred(*first1, *first2) == false )
  {
    ++first1;
    ++first2;
  }
  return pair<InputIterator1, InputIterator2>(first1, first2);
}


template<class InputIterator1, class InputIterator2>
__forceinline
bool
  equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
{
  for ( ; first1 != last1; ++first1, ++first2 )
    if ( !(*first1 == *first2) ) return false;
  return true;
}


template<class InputIterator1, class InputIterator2, class BinaryPredicate>
__forceinline
bool
  equal(InputIterator1 first1, InputIterator1 last1,
        InputIterator2 first2, BinaryPredicate pred)
{
  for ( ; first1 != last1; ++first1, ++first2 )
    if ( !pred(*first1, *first2) ) return false;
  return true;
}

template<class InputIterator1, class InputIterator2>
__forceinline
bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
{
  for ( ; first1 != last1 && first2 != last2; ++first1, ++first2 )
    if ( !(*first1 == *first2) ) return false;
  return true;
}


template<class InputIterator1, class InputIterator2, class BinaryPredicate>
__forceinline
bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate pred)
{
  for ( ; first1 != last1 && first2 != last2; ++first1, ++first2 )
    if ( !pred(*first1, *first2) ) return false;
  return true;
}

template<class ForwardIterator1, class ForwardIterator2>
__forceinline
ForwardIterator1
  search(ForwardIterator1 first1, ForwardIterator1 last1,
         ForwardIterator2 first2, ForwardIterator2 last2)
{
  // iterators are of forward category, so do a brute force search
  // @todo: specializations for other iterator categories?
  for ( ; ; )
  {
    ForwardIterator1 i1 = first1;
    ForwardIterator2 i2 = first2;
    for ( ; ; )
    {
      if ( i1 == last1 ) return first1; // not found, first1 == last1
      if ( i2 == last2 ) return first1; // found
      if ( *i1 != *i2 ) break;
      ++i1;
      ++i2;
    }
    ++first1;
  }
}


template<class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
__forceinline
ForwardIterator1
  search(ForwardIterator1 first1, ForwardIterator1 last1,
         ForwardIterator2 first2, ForwardIterator2 last2,
         BinaryPredicate pred)
{
  for ( ; ; )
  {
    ForwardIterator1 i1 = first1;
    ForwardIterator2 i2 = first2;
    for ( ; ; )
    {
      if ( i1 == last1 ) return first1; // not found, first1 == last1
      if ( i2 == last2 ) return first1; // found
      if ( pred(*i1, *i2) != false ) break;
      ++i1;
      ++i2;
    }
    ++first1;
  }
}


template<class ForwardIterator, class Size, class T>
__forceinline
ForwardIterator
  search_n(ForwardIterator first, ForwardIterator last, Size count,
           const T& value)
{
  for ( ; ; )
  {
    ForwardIterator i = first;
    Size n = count;
    for ( ; ; )
    {
      if ( i == last ) return first; // not found, first == last
      if ( n == 0 ) return first; // found
      if ( *i == value ) break;
      ++i;
      --n;
    }
    ++first;
  }
}


template<class ForwardIterator, class Size, class T, class BinaryPredicate>
__forceinline
ForwardIterator
  search_n(ForwardIterator first, ForwardIterator last,
           Size count, const T& value, BinaryPredicate pred)
{
  for ( ; ; )
  {
    ForwardIterator i = first;
    Size n = count;
    for ( ; ; )
    {
      if ( i == last ) return first; // not found, first == last
      if ( n == 0 ) return first; // found
      if ( pred(*i, value) != false ) break;
      ++i;
      --n;
    }
    ++first;
  }
}

///\name 25.2, modifying sequence operations:


template<class InputIterator, class OutputIterator>
__forceinline
OutputIterator
  copy(InputIterator first, InputIterator last, OutputIterator result)
{
  while ( first != last )
  {
    *result = *first;
    ++result;
    ++first;
  }
  return result;
}

namespace __
{
  template<class InputIterator, class Size, class OutputIterator>
  inline OutputIterator copy_n(InputIterator first, Size n, OutputIterator result, const input_iterator_tag&)
  {
    for(; n; --n, ++first, ++result)
    {
      *result = *first;
    }
    return result;
  }

  template<class RandomAccessIterator, class Size, class OutputIterator>
  inline OutputIterator copy_n(RandomAccessIterator first, Size n, OutputIterator result, const random_access_iterator_tag&)
  {
    return copy(first, first+n, result);
  }

  template<class InputIterator, class OutputIterator>
  inline OutputIterator move_impl(InputIterator first, InputIterator last, OutputIterator result, false_type)
  {
    while ( first != last )
    {
      *result = std::move(*first);
      ++result;
      ++first;
    }
    return result;
  }

  template<class InputRandomIterator, class OutputRandomIterator>
  inline OutputRandomIterator move_impl(InputRandomIterator first, InputRandomIterator last, OutputRandomIterator result, true_type)
  {
    memmove(result, first, (last - first)*sizeof(typename iterator_traits<InputRandomIterator>::value_type));
    return result + (last - first);
  }

  template<class I, class O = I>
  struct is_memmoveable_check
  {
    typedef typename iterator_traits<I>::iterator_category itype;
    typedef typename iterator_traits<O>::iterator_category otype;
    typedef typename iterator_traits<I>::value_type src_type;
    typedef typename iterator_traits<O>::value_type dest_type;

    // if iterators is pointers and value types memmoveable, just move it
    typedef bool_type<is_pointer<I>::value && is_pointer<O>::value> ptrs;
    static const bool same = is_same<typename remove_cv<src_type>::type, typename remove_cv<dest_type>::type>::value;
    static const bool value = ptrs::value && same && has_trivial_copy_constructor<dest_type>::value;
  };

  template<class I, class O>
  struct is_memmoveable:
    integral_constant<bool, is_memmoveable_check<I,O>::value>
  {};


}
template<class InputIterator, class Size, class OutputIterator>
inline OutputIterator
  copy_n(InputIterator first, Size n, OutputIterator result)
{
  return __::copy_n(first, n, result, iterator_traits<InputIterator>::iterator_category());
}

template<class InputIterator, class OutputIterator, class Predicate>
inline OutputIterator
  copy_if(InputIterator first, InputIterator last,
          OutputIterator result, Predicate pred)
{
  for(; first != last; ++first, ++result)
  {
    if(pred(*first) == true)
      *result = *first;
  }
}


template<class BidirectionalIterator1, class BidirectionalIterator2>
__forceinline
BidirectionalIterator2
  copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last,
                BidirectionalIterator2 result_last)
{
  while ( first != last ) *--result_last = *--last;
  return result_last;
}


template<class InputIterator, class OutputIterator>
inline OutputIterator move(InputIterator first, InputIterator last, OutputIterator result)
{
  return __::move_impl<InputIterator,OutputIterator>(first, last, result, __::is_memmoveable<InputIterator,OutputIterator>());
}

template<class BidirectionalIterator1, class BidirectionalIterator2>
inline BidirectionalIterator2
  move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last,
                BidirectionalIterator2 result_last)
{
  while ( first != last ) *--result_last = move(*--last);
  return result_last;
}


template<class T>
inline void swap(T& a, T& b)
{
#ifndef NTL_CXX_RV
  T temp( a );
  a = b;
  b = temp;
#else
  T temp(move(a));
  a = move(b);
  b = move(temp);
#endif
}


template <class T, size_t N>
inline void swap(T (&a)[N], T (&b)[N])
{
  swap_ranges(a, a+N, b);
}


#ifndef NTL_CXX_RV
template <class T, class U> inline T exchange(T& obj, U& new_val)
{
  T old_val(obj);
  obj = new_val;
  return old_val;
}
template <class T, class U> inline T exchange(T& obj, const U& new_val)
{
  T old_val(obj);
  obj = new_val;
  return old_val;
}
#else

template <class T, class U> 
inline T exchange(T& obj, U&& new_val)
{
  T old_val = std::move(obj);
  obj = std::forward<U>(new_val);
  return old_val;
}
#endif

template<class ForwardIterator1, class ForwardIterator2>
__forceinline
ForwardIterator2
  swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1,
              ForwardIterator2 first2)
{
  for ( ; first1 != last1; ++first1, ++first2 )
    swap(*first1, *first2);
  return first2;
}


template<class ForwardIterator1, class ForwardIterator2>
__forceinline
void
  iter_swap(ForwardIterator1 a, ForwardIterator2 b)
{
  swap(*a, *b);
}


template<class InputIterator, class OutputIterator, class UnaryOperation>
__forceinline
OutputIterator
  transform(InputIterator first, InputIterator last,
            OutputIterator result, UnaryOperation op)
{
  for ( ; first != last; ++first, ++result )
    *result = op(*first);
  return result;
}


template<class InputIterator1, class InputIterator2, class OutputIterator,
         class BinaryOperation>
__forceinline
OutputIterator
  transform(InputIterator1 first1, InputIterator1 last1,
            InputIterator2 first2, OutputIterator result,
            BinaryOperation binary_op)
{
  for ( ; first1 != last1; ++first1, ++first2, ++result )
    *result = binary_op(*first1, *first2);
  return result;
}


template<class ForwardIterator, class T>
__forceinline
void
  replace(ForwardIterator first, ForwardIterator last,
          const T& old_value, const T& new_value)
{
  for ( ; first != last; ++first )
    if ( *first == old_value ) *first = new_value;
}


template<class ForwardIterator, class Predicate, class T>
__forceinline
void
  replace_if(ForwardIterator first, ForwardIterator last,
             Predicate pred, const T& new_value)
{
  for ( ; first != last; ++first )
    if ( pred(*first) != false ) *first = new_value;
}


template<class InputIterator, class OutputIterator, class T>
__forceinline
OutputIterator
  replace_copy(InputIterator first, InputIterator last, OutputIterator result,
               const T& old_value, const T& new_value)
{
  for ( ; first != last; ++first, ++result )
    if ( *result == old_value ) *result = new_value;
}


template<class Iterator, class OutputIterator, class Predicate, class T>
__forceinline
OutputIterator
  replace_copy_if(Iterator first, Iterator last, OutputIterator result,
                  Predicate pred, const T& new_value)
{
  for ( first != last; ++first; ++result )
    if ( pred(*result) != false ) *result = new_value;
}

namespace __
{
  
  template<class OutputIterator, class Size, class T>
  __forceinline OutputIterator fill_n(OutputIterator first, Size n, const T& value)
  {
    for ( ; n--; ++first ) *first = value;
    return first;
  }
}


template<class ForwardIterator, class T>
__forceinline
void
  fill(ForwardIterator first, ForwardIterator last, const T& value)
{
  for ( ; first != last; ++first ) *first = value;
}


template<class OutputIterator, class Size, class T>
__forceinline
void
  fill_n(OutputIterator first, Size n, const T& value)
{
  for ( ; n--; ++first ) *first = value;
}


template<class ForwardIterator, class Generator>
__forceinline
void
  generate(ForwardIterator first, ForwardIterator last, Generator gen)
{
  for ( ; first != last; ++first ) *first = gen();
}


template<class OutputIterator, class Size, class Generator>
__forceinline
void
  generate_n(OutputIterator first, Size n, Generator gen)
{
  for ( ; n--; ++first ) *first = gen();
}

template<class InputIterator, class OutputIterator, class T>
__forceinline
OutputIterator
  remove_copy(InputIterator first, InputIterator last,
              OutputIterator result, const T& value)
{
  for ( ; first != last; ++first )
    if ( !(*first == value) )
    {
      *result == *first;
      ++result;
    }
  return result;
}


template<class InputIterator, class OutputIterator, class Predicate>
__forceinline
OutputIterator
  remove_copy_if(InputIterator first, InputIterator last,
                 OutputIterator result, Predicate pred)
{
  for ( ; first != last; ++first )
    if ( !pred(*first) )
    {
      *result = *first;
      ++result;
    }
  return result;
}


template<class ForwardIterator, class T>
__forceinline
ForwardIterator
  remove(ForwardIterator first, ForwardIterator last, const T& value)
{
  first = find(first, last, value);
  ForwardIterator i = first;
  i = ( i == last ) ? i : remove_copy(++i, last, first, value);
  return i;
}


template<class ForwardIterator, class Predicate>
__forceinline
ForwardIterator
  remove_if(ForwardIterator first, ForwardIterator last, Predicate pred)
{
  first = find_if(first, last, pred);
  ForwardIterator i = first;
  i = ( i == last ) ? i : remove_copy_if(++i, last, first, pred);
  return i;
}


template<class InputIterator, class OutputIterator>
__forceinline
OutputIterator
  unique_copy(InputIterator first, InputIterator last, OutputIterator result)
{
  while ( first != last )
  {
    typename iterator_traits<OutputIterator>::value_type unique = *result = *first;
    ++result;
    while ( ++first != last && unique == *first ) {/*-*/}
  }
  return result;
}


template<class InputIterator, class OutputIterator, class BinaryPredicate>
__forceinline
OutputIterator
  unique_copy(InputIterator first, InputIterator last,
              OutputIterator result, BinaryPredicate pred)
{
  while ( first != last )
  {
    typename iterator_traits<OutputIterator>::value_type unique = *result = *first;
    ++result;
    while ( ++first != last && pred(unique, *first) != false ) {/*-*/}
  }
  return result;
}


template<class ForwardIterator>
__forceinline
ForwardIterator
  unique(ForwardIterator first, ForwardIterator last)
{
  first = adjacent_find(first, last);
  return unique_copy(first, last, first);
}


template<class ForwardIterator, class BinaryPredicate>
__forceinline
ForwardIterator
  unique(ForwardIterator first, ForwardIterator last, BinaryPredicate pred)
{
  first = adjacent_find(first, last, pred);
  return unique_copy(first, last, first, pred);
}

namespace __
{
  template<class BidirectionalIterator> __forceinline void reverse(BidirectionalIterator first, BidirectionalIterator last, bidirectional_iterator_tag)
  {
    for ( ; first != last && first != --last; ++first )
      iter_swap(first, last);
  }
  template<class RandomAccessIterator> __forceinline void reverse(RandomAccessIterator first, RandomAccessIterator last, random_access_iterator_tag)
  {
    if(first == last)
      return;
    for(--last; first < last; ++first, --last)
      iter_swap(first, last);
  }
}

template<class BidirectionalIterator>
__forceinline void reverse(BidirectionalIterator first, BidirectionalIterator last)
{
  __::reverse(first, last, std::iterator_traits<BidirectionalIterator>::iterator_category());
}


template<class BidirectionalIterator, class OutputIterator>
__forceinline
OutputIterator
  reverse_copy(BidirectionalIterator first, BidirectionalIterator last,
               OutputIterator result)
{
  for ( ; first != last; ++result )
    *result = *--last;
  return result;
}


template<class ForwardIterator>
inline
void
  rotate(ForwardIterator first, ForwardIterator middle, ForwardIterator last);

template<class ForwardIterator, class OutputIterator>
inline
OutputIterator
  rotate_copy(ForwardIterator first, ForwardIterator middle,
              ForwardIterator last, OutputIterator result);

template<class RandomAccessIterator>
inline
void
  random_shuffle(RandomAccessIterator first, RandomAccessIterator last);

#ifdef NTL_CXX_RV
template<class RandomAccessIterator, class RandomNumberGenerator>
inline
void
  random_shuffle(RandomAccessIterator first, RandomAccessIterator last,
                 RandomNumberGenerator&& rand);
#else
template<class RandomAccessIterator, class RandomNumberGenerator>
inline
void
  random_shuffle(RandomAccessIterator first, RandomAccessIterator last,
                 RandomNumberGenerator& rand);
#endif

template<class RandomAccessIterator, class UniformRandomNumberGenerator>
inline
void
  random_shuffle(RandomAccessIterator first,
                 RandomAccessIterator last,
                 UniformRandomNumberGenerator& g);

///\name 25.2.13, partitions:
template <class InputIterator, class Predicate>
bool is_partitioned(InputIterator first, InputIterator last, Predicate pred);

template<class BidirectionalIterator, class Predicate>
inline
BidirectionalIterator
  partition(BidirectionalIterator first, BidirectionalIterator last,
            Predicate pred);

template<class BidirectionalIterator, class Predicate>
inline
BidirectionalIterator
  stable_partition(BidirectionalIterator first, BidirectionalIterator last,
                   Predicate pred);
template <class InputIterator, class OutputIterator1,
class OutputIterator2, class Predicate>
  pair<OutputIterator1, OutputIterator2>
  partition_copy(InputIterator first, InputIterator last,
  OutputIterator1 out_true, OutputIterator2 out_false,
  Predicate pred);

template<class ForwardIterator, class Predicate>
ForwardIterator partition_point(ForwardIterator first,
                                ForwardIterator last,
                                Predicate pred);

///\name 25.3, sorting and related operations:
///\name 25.3.1, sorting:
template<class RandomAccessIterator>
inline void sort(RandomAccessIterator first, RandomAccessIterator last)
{
  typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
  sort (first, last, less<value_type>());
}

template<class RandomAccessIterator, class Compare>
inline void sort(RandomAccessIterator first, RandomAccessIterator last, Compare /*comp*/)
{
  typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
  typedef const value_type* const_pointer;

  struct proxy
  {
    static int NTL_CRTIMP qsort(const void* a, const void* b)
    {
      const_pointer x = reinterpret_cast<const_pointer>(a), y = reinterpret_cast<const_pointer>(b);
      Compare comp;
      return comp(*x, *y) ? -1 : (comp(*y,*x) ? 1 : 0);
    }
  };
  qsort(first, distance(first, last), sizeof(value_type), &proxy::qsort);
}

template<class RandomAccessIterator>
inline
void
  stable_sort(RandomAccessIterator first, RandomAccessIterator last);

template<class RandomAccessIterator, class Compare>
inline
void
  stable_sort(RandomAccessIterator first, RandomAccessIterator last,
              Compare comp);

template<class RandomAccessIterator>
inline
void
  partial_sort(RandomAccessIterator first, RandomAccessIterator middle,
               RandomAccessIterator last);

template<class RandomAccessIterator, class Compare>
inline
void
  partial_sort(RandomAccessIterator first, RandomAccessIterator middle,
               RandomAccessIterator last, Compare comp);

template<class InputIterator, class RandomAccessIterator>
inline
RandomAccessIterator
  partial_sort_copy(InputIterator first, InputIterator last,
                    RandomAccessIterator result_first,
                    RandomAccessIterator result_last);

template<class InputIterator, class RandomAccessIterator, class Compare>
inline
RandomAccessIterator
  partial_sort_copy(InputIterator first, InputIterator last,
                    RandomAccessIterator result_first,
                    RandomAccessIterator result_last,
                    Compare comp);

template<class ForwardIterator>
bool is_sorted(ForwardIterator first, ForwardIterator last);
template<class ForwardIterator, class Compare>
bool is_sorted(ForwardIterator first, ForwardIterator last,
               Compare comp);
template<class ForwardIterator>
ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last);
template<class ForwardIterator, class Compare>
ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last,
                                Compare comp);

template<class RandomAccessIterator>
inline
void
  nth_element(RandomAccessIterator first, RandomAccessIterator nth,
              RandomAccessIterator last);

template<class RandomAccessIterator, class Compare>
inline
void
  nth_element(RandomAccessIterator first, RandomAccessIterator nth,
              RandomAccessIterator last, Compare comp);

///\name 25.3.3, binary search:

template<class ForwardIterator, class T, class Compare>
inline
ForwardIterator
  lower_bound(ForwardIterator first, ForwardIterator last,
              const T& value, Compare comp)
{
  typedef typename iterator_traits<ForwardIterator>::difference_type diff_t; 
  diff_t len = distance(first, last);
  while ( len > 0 )
  {
    diff_t const half = len / 2;
    ForwardIterator middle = first;
    advance(middle, half);
    if ( comp(*middle, value) != false )
    {
      first = middle;
      ++first;
      len = len - half - 1;
    }
    else
    {
      len = half;
    }
  }
  return first;
}

template<class ForwardIterator, class T>
inline
ForwardIterator
  lower_bound(ForwardIterator first, ForwardIterator last, const T& value)
{
  return lower_bound(first, last, value, less<T>());
}

template<class ForwardIterator, class T>
inline
ForwardIterator
  upper_bound(ForwardIterator first, ForwardIterator last, const T& value)
{
  return upper_bound(first, last, value, less<T>());
}

template<class ForwardIterator, class T, class Compare>
inline
ForwardIterator
  upper_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
{
  typedef typename iterator_traits<ForwardIterator>::difference_type diff_t;
  diff_t len = std::distance(first, last);
  while(len > 0){
    const diff_t half = len / 2;
    ForwardIterator split = first;
    std::advance(split, half);
    if(comp(value, *split) != false){
      len = half;
    }else{
      first = split;
      ++first;
      len = len - half - 1;
    }
  }
  return first;
}

template<class ForwardIterator, class T>
inline
pair<ForwardIterator, ForwardIterator>
  equal_range(ForwardIterator first, ForwardIterator last, const T& value);

template<class ForwardIterator, class T, class Compare>
inline
pair<ForwardIterator, ForwardIterator>
  equal_range(ForwardIterator first, ForwardIterator last,
              const T& value, Compare comp);

template<class ForwardIterator, class T>
inline bool binary_search(ForwardIterator first, ForwardIterator last, const T& value)
{
  ForwardIterator i = std::lower_bound(first, last, value);
  return i != last && !(value < *i);
}

template<class ForwardIterator, class T, class Compare>
inline bool binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
{
  ForwardIterator i = std::lower_bound(first, last, value, comp);
  return i != last && comp(value, *i) == false;

}

///\name 25.3.4, merge:
template<class InputIterator1, class InputIterator2, class OutputIterator>
inline
OutputIterator
  merge(InputIterator1 first1, InputIterator1 last1,
        InputIterator2 first2, InputIterator2 last2,
        OutputIterator result);

template<class InputIterator1, class InputIterator2, class OutputIterator,
         class Compare>
inline
OutputIterator
  merge(InputIterator1 first1, InputIterator1 last1,
        InputIterator2 first2, InputIterator2 last2,
        OutputIterator result, Compare comp);

template<class BidirectionalIterator>
inline
void
  inplace_merge(BidirectionalIterator first, BidirectionalIterator middle,
                BidirectionalIterator last);

template<class BidirectionalIterator, class Compare>
inline
void
  inplace_merge(BidirectionalIterator first, BidirectionalIterator middle,
                BidirectionalIterator last, Compare comp);

///\name 25.3.5, set operations on sorted structures:
template<class InputIterator1, class InputIterator2>
inline
bool
  includes(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, InputIterator2 last2);

template<class InputIterator1, class InputIterator2, class Compare>
inline
bool
  includes(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, InputIterator2 last2, Compare comp);

template<class InputIterator1, class InputIterator2, class OutputIterator>
inline
OutputIterator
  set_union(InputIterator1 first1, InputIterator1 last1,
            InputIterator2 first2, InputIterator2 last2,
            OutputIterator result);

template<class InputIterator1, class InputIterator2, class OutputIterator,
         class Compare>
inline
OutputIterator
  set_union(InputIterator1 first1, InputIterator1 last1,
            InputIterator2 first2, InputIterator2 last2,
            OutputIterator result, Compare comp);

template<class InputIterator1, class InputIterator2, class OutputIterator>
inline
OutputIterator
  set_intersection(InputIterator1 first1, InputIterator1 last1,
                   InputIterator2 first2, InputIterator2 last2,
                   OutputIterator result);

template<class InputIterator1, class InputIterator2, class OutputIterator,
         class Compare>
inline
OutputIterator
  set_intersection(InputIterator1 first1, InputIterator1 last1,
                   InputIterator2 first2, InputIterator2 last2,
                   OutputIterator result, Compare comp);

template<class InputIterator1, class InputIterator2, class OutputIterator>
inline
OutputIterator
  set_difference(InputIterator1 first1, InputIterator1 last1,
                 InputIterator2 first2, InputIterator2 last2,
                 OutputIterator result);

template<class InputIterator1, class InputIterator2, class OutputIterator,
         class Compare>
inline
OutputIterator
  set_difference(InputIterator1 first1, InputIterator1 last1,
                 InputIterator2 first2, InputIterator2 last2,
                 OutputIterator result, Compare comp);

template<class InputIterator1, class InputIterator2, class OutputIterator>
inline
OutputIterator
  set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
                           InputIterator2 first2, InputIterator2 last2,
                           OutputIterator result);

template<class InputIterator1, class InputIterator2, class OutputIterator,
         class Compare>
inline
OutputIterator
  set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
                           InputIterator2 first2, InputIterator2 last2,
                           OutputIterator result, Compare comp);

///\name 25.3.6, heap operations:

namespace __
{
  template<class Compare, class RandomAccessIterator>
  inline void push_heap_front(RandomAccessIterator first, RandomAccessIterator last, Compare comp, typename iterator_traits<RandomAccessIterator>::difference_type count)
  {
    typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
    if(count <= 1)
      return;
    
    difference_type p = 0, c = 2;
    RandomAccessIterator pp = first, cp = first + c;

    if(c == count || comp(*cp, *(cp - 1))) {
      --c;
      --cp;
    }
    if(comp(*pp, *cp)) {
      value_type tmp = std::move(*pp);
      do {
        *pp = std::move(*cp);
        pp = cp;
        p = c;
        c = (p + 1) * 2;
        if(c > count)
          break;

        cp = first + c;
        if(c == count || comp(*cp, *(cp - 1))) {
          --c;
          --cp;
        }
      } while(comp(tmp, *cp));
      *pp = std::move(tmp);
    }
  }

  template<class Compare, class RandomAccessIterator>
  inline void push_heap_back(RandomAccessIterator first, RandomAccessIterator last, Compare comp, typename iterator_traits<RandomAccessIterator>::difference_type count)
  {
    typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
    if(count <= 1)
      return;

    count = (count - 2) / 2;
    RandomAccessIterator ptr = first + count;
    if(comp(*ptr, *--last)) {
      value_type tmp = std::move(*last);
      do {
        *last = std::move(*ptr);
        last = ptr;
        if(count == 0)
          break;

        count = (count - 1) / 2;
        ptr = first + count;
      } while(comp(*ptr, tmp));
      *last = std::move(tmp);
    }
  }

  template<class Compare, class RandomAccessIterator>
  inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp, typename iterator_traits<RandomAccessIterator>::difference_type count)
  {
    if(count > 1) {
      swap(*first, *--last);
      push_heap_front<Compare>(first, last, comp, count-1);
    }
  }

}

template<class RandomAccessIterator, class Compare>
inline void push_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
  __::push_heap_back<Compare>(first, last, comp, last - first);
}

template<class RandomAccessIterator>
inline void push_heap(RandomAccessIterator first, RandomAccessIterator last)
{
  push_heap(first, last, less<typename iterator_traits<RandomAccessIterator>::value_type>());
}

template<class RandomAccessIterator, class Compare>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
  __::pop_heap<Compare>(first, last, comp, last-first);
}

template<class RandomAccessIterator>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last)
{
  pop_heap(first, last, less<typename iterator_traits<RandomAccessIterator>::value_type>());
}

template<class RandomAccessIterator, class Compare>
inline void make_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
  typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
  difference_type n = last - first;
  if(n > 1) {
    last = first;
    ++last;
    for(difference_type i = 1; i < n;) {
      __::push_heap_back<Compare>(first, ++last, comp, ++i);
    }
  }
}

template<class RandomAccessIterator>
inline void make_heap(RandomAccessIterator first, RandomAccessIterator last)
{
  return make_heap(first, last, less<typename iterator_traits<RandomAccessIterator>::value_type>());
}

template<class RandomAccessIterator, class Compare>
inline void sort_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
  typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
  for(difference_type n = last - first; n > 1; --last, --n)
    __::pop_heap<Compare>(first, last, comp, n);
}

template<class RandomAccessIterator>
inline void sort_heap(RandomAccessIterator first, RandomAccessIterator last)
{
  sort_heap(first, last, less<typename iterator_traits<RandomAccessIterator>::value_type>());
}

template<class RandomAccessIterator>
inline bool is_heap(RandomAccessIterator first, RandomAccessIterator last);

template<class RandomAccessIterator, class Compare>
inline bool is_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp);

template<class RandomAccessIterator>
inline RandomAccessIterator is_heap_until(RandomAccessIterator first, RandomAccessIterator last);

template<class RandomAccessIterator, class Compare>
inline RandomAccessIterator is_heap_until(RandomAccessIterator first, RandomAccessIterator last, Compare comp);


///\name 25.4.7 Minimum and maximum [alg.min.max]
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

template<class T>
__forceinline const T& min(const T& a, const T& b)
{
  return ( b < a ) ? b : a;
}

template<class T, class Compare>
__forceinline const T& min(const T& a, const T& b, Compare comp)
{
  return comp(b, a) ? b : a;
}

template<class T>
inline T min(initializer_list<T> t)
{
  return *min_element(t.begin(), t.end());
}

template<class T, class Compare>
inline T min(initializer_list<T> t, Compare comp)
{
  return *min_element(t.begin(), t.end(), comp);
}

template<class T>
__forceinline
const T& max(const T& a, const T& b)
{
  return ( a < b ) ? b : a;
}

template<class T, class Compare>
__forceinline const T& max(const T& a, const T& b, Compare comp)
{
  return comp(a, b) ? b : a;
}

template <class T>
inline T max(initializer_list<T> t)
{
  return *max_element(t.begin(), t.end());
}

template<class T, class Compare>
inline T max(initializer_list<T> t, Compare comp)
{
  return *max_element(t.begin(), t.end(), comp);
}

/**	\return <tt>pair<const T&, const T&>(b, a)</tt> if \p b is smaller than \p a, and <tt>pair<const T&, constT&>(a, b)</tt> otherwise. */
template<class T> 
inline pair<const T&, const T&> minmax(const T& a, const T& b)
{
  typedef pair<const T&,const T&> pair;
  return b < a ? pair(a,b) : pair(b,a);
}

/**	\return <tt>pair<const T&, const T&>(b, a)</tt> if \p b is smaller than \p a, and <tt>pair<const T&, constT&>(a, b)</tt> otherwise. */
template<class T, class Compare>
inline pair<const T&, const T&> minmax(const T& a, const T& b, Compare comp)
{
  typedef pair<const T&,const T&> pair;
  return comp(b,a) ? pair(a,b) : pair(b,a);
}

template<class T>
inline pair<const T&, const T&> minmax(initializer_list<T> t)
{
  const pair<const T*, const T*> re = minmax_element(t.begin(), t.end());
  return pair<const T&, const T&>(*re.first, *re.second);
}

template<class T, class Compare>
inline pair<const T&, const T&> minmax(initializer_list<T> t, Compare comp)
{
  const pair<const T*, const T*> re = minmax_element(t.begin(), t.end(), comp);
  return pair<const T&, const T&>(*re.first, *re.second);
}

template<class ForwardIterator>
inline
ForwardIterator
  min_element(ForwardIterator first, ForwardIterator last);

template<class ForwardIterator, class Compare>
inline
ForwardIterator
  min_element(ForwardIterator first, ForwardIterator last, Compare comp);

template<class ForwardIterator>
inline
ForwardIterator
  max_element(ForwardIterator first, ForwardIterator last);

template<class ForwardIterator, class Compare>
inline
ForwardIterator
  max_element(ForwardIterator first, ForwardIterator last, Compare comp);

template<class ForwardIterator, class Compare>
inline pair<ForwardIterator, ForwardIterator> minmax_element(ForwardIterator first, ForwardIterator last, Compare comp)
{
  pair<ForwardIterator, ForwardIterator> re(first, first);
  if(first != last) {
    if(++first != last) {
      if(comp(*first, *re.first))
        re.first = first;
      else
        re.second = first;

      while(++first != last) {
        ForwardIterator i = first;
        if(++first == last) {
          // last pass
          if(comp(*i, *re.first))
            re.first = i;
          else if(!comp(*i, *re.second))
            re.second = i;
          break;
        }

        if(comp(*first, *i)) {
          if(comp(*first, *re.first))
            re.first = first;
          if(!comp(*i, *re.second))
            re.second = i;
        } else {
          if(comp(*i, *re.first))
            re.first = i;
          if(!comp(*first, *re.second))
            re.second = first;
        }
      }
    }
  }
  return re;
}

template<class ForwardIterator>
inline pair<ForwardIterator, ForwardIterator> minmax_element(ForwardIterator first, ForwardIterator last)
{
  return minmax_element(first, last, less<typename iterator_traits<ForwardIterator>::value_type>());
}

///\name 25.3.8 Lexicographical comparison [alg.lex.comparison]
template<class InputIterator1, class InputIterator2>
__forceinline bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                          InputIterator2 first2, InputIterator2 last2)
{
  for ( ; first1 != last1; ++first1, ++first2 )
    if ( first2 == last2 )   return false;
    else
    if ( *first1 < *first2 ) return true;
    else
    if ( *first2 < *first1 ) return false;
  return first2 != last2;
}


template<class InputIterator1, class InputIterator2, class Compare>
__forceinline bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                          InputIterator2 first2, InputIterator2 last2,
                          Compare comp)
{
  for ( ; first1 != last1; ++first1, ++first2 )
    if ( first2 == last2 )        return false;
    else
    if ( comp(*first1, *first2) ) return true;
    else
    if ( comp(*first2, *first1) ) return false;
  return first2 != last2;
}

///\name 25.3.9 Permutation generators [alg.permutation.generators]
template<class BidirectionalIterator>
inline
bool
  next_permutation(BidirectionalIterator first, BidirectionalIterator last);

template<class BidirectionalIterator, class Compare>
inline
bool
  next_permutation(BidirectionalIterator first, BidirectionalIterator last,
                   Compare comp);

template<class BidirectionalIterator>
inline
bool
  prev_permutation(BidirectionalIterator first, BidirectionalIterator last);

template<class BidirectionalIterator, class Compare>
inline
bool
  prev_permutation(BidirectionalIterator first, BidirectionalIterator last,
                   Compare comp);

}//namespace std

#endif // NTL__STLX_ALGORITHM

// This file is derived from HP STL and STLPort:
/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */
