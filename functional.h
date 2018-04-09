//* functional.h - extra stl container functions & functors header file.
/*
 * @brief header file for extra stl container functions & functors.
 */
#ifndef __IF_FUNCTIONAL__
#define __IF_FUNCTIONAL__
#include <functional>

/*
 * @brief this namespace is for items that are of general interest.
 */
namespace re_gen {

//* copy_if function
/*
 * @brief copy_if function
 * @param iterator src_begin - iterator to first element to copy from source container.
 * @param iterator src_end - iterator to one past last element to copy from source
 * container.
 * @param iterator dst_begin - iterator to first element to copy into in destination
 * container.
 * @param predicate - predicate function or functor.
 * @returns iterator one past last element copied in destination container.
 * @remarks this function copies container elements from one container to another. only
 * elements for which the predicate returns
 * true are copied.
 * @example:
 *  class a { ... };
 *  vector<a> va, vb;
 *  copy_if<a>(a.begin(), a.end(), back_inserter(vb), predicate<a>);
 * @note the predicate is a function or object with a suitable override pf the ()
 * operator. the function (or overridden () operator) must take a single source container
 * element parameter and return a bool.
 * @note this function really ought to be part of the stl. i figured at least we'd
 * implement it in our own general library.
 */
template <typename In_T, typename Out_T, typename Condition>
  Out_T copy_if(In_T in, In_T in_end, Out_T out, Condition condition) {
  while (in != in_end) {
    if (condition(*in))
      *out++ = *in;
    ++in;
  }
  return(out);
}

//* cast function
/*
 * @brief cast function
 * @param iterator src_begin - iterator to first element to copy from source container.
 * @param iterator src_end - iterator to one past last element to copy from source
 * container.
 * @param iterator dst_begin - iterator to first element to copy into in destination
 * container.
 * @returns iterator one past last element copied in destination container.
 * @remarks this function copies container elements from one container to another,
 * performing a dynamic cast during the copy.
 * @example:
 *  class a { ... };
 *  class b : a { ... };
 *  vector<b *> vb;
 *  vector<a *> va;
 *  cast<a>(b.begin(), b.end(), back_inserter(va));
 */
template <typename Cast_T>
class cast {
public:
  cast(void) {
  }
  template <typename In_T, typename Out_T>
  Out_T operator()(In_T in, In_T in_end, Out_T out) {
    while (in != in_end) {
      *out++ = dynamic_cast<Cast_T>(*in);
      ++in;
    }
    return(out);
  }
};

//* cast_if function
/*
 * @brief cast_if function
 * @param iterator src_begin - iterator to first element to copy from source container.
 * @param iterator src_end - iterator to one past last element to copy from source
 * container.
 * @param iterator dst_begin - iterator to first element to copy into in destination
 * container.
 * @param predicate - predicate function or functor.
 * @returns iterator one past last element copied in destination container.
 * @remarks this function copies container elements from one container to another,
 * performing a dynamic cast during the copy. only elements for which the predicate returns
 * true are copied.
 * @example:
 *  class a { ... };
 *  class b : a { ... };
 *  vector<b *> vb;
 *  vector<a *> va;
 *  cast<a>(b.begin(), b.end(), back_inserter(va), predicate<b *>);
 * @note the predicate is a function or object with a suitable override pf the ()
 * operator. the function (or overridden () operator) must take a single source container
 * element parameter and return a bool.
 * @see cast, copy_if
 */
template <typename Cast_T>
class cast_if {
public:
  cast_if(void) {
  }
  template <typename In_T, typename Out_T, typename Condition>
  Out_T operator()(In_T in, In_T in_end, Out_T out, Condition condition) {
    while (in != in_end) {
      if (condition(*in))
	*out++ = dynamic_cast<Cast_T>(*in);
      ++in;
    }
    return(out);
  }
};

//* split_if function
/*
 * @brief split_if function
 * @param iterator src_begin - iterator to first element to copy from source container.
 * @param iterator src_end - iterator to one past last element to copy from source
 * container.
 * @param iterator first_dst_begin - iterator to first element to copy into, in first
 * destination container.
 * @param iterator second_dst_begin - iterator to first element to copy into, in second
 * destination container.
 * @param predicate - predicate function or functor.
 * @returns iterator one past last element copied in first destination container.
 * @remarks this function copies container elements from one container to two
 * others. elements for which the predicate returns true are copied into the first
 * destination container. elements for which the predicate returns false are copied into
 * the second destination container.
 * @example:
 *  class a { ... };
 *  vector<a> va, vb, vc;
 *  split_if<a>(a.begin(), a.end(), back_inserter(vb), back_inserter(vc), predicate<a>);
 * @note the predicate is a function or object with a suitable override pf the ()
 * operator. the function (or overridden () operator) must take a single source container
 * element parameter and return a bool.
 * @see copy_if
 */
template <typename In_T, typename Out_T1, typename Out_T2, typename Condition>
  Out_T1 split_if(In_T in, In_T in_end, Out_T1 out1, Out_T2 out2, Condition condition) {
  while (in != in_end) {
    if (condition(*in))
      *out1++ = *in;
    else
      *out2++ = *in;
    ++in;
  }
  return(out1);
}
};//namespace re_gen
#endif //__IF_FUNCTIONAL__

