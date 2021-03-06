/**\file*********************************************************************
 *                                                                     \brief
 *  new and delete operators
 *
 ****************************************************************************
 */
#ifndef NTL__KM_NEW
#define NTL__KM_NEW
#pragma once

namespace ntl {
  namespace km {
    __declspec(selectany)
      extern
      const std::nothrow_t nonpaged;
  }
}

#ifndef NTL_NO_NEW

#include "pool.hxx"
#include "../stlx/new.hxx"

///\name  Single-object forms

__forceinline
void * __cdecl
  operator new(std::size_t size))
{
  return ntl::km::pool<ntl::km::PagedPool>::alloc(size);
}

__forceinline
void __cdecl
  operator delete(void* ptr) __ntl_nothrow
{
  if ( ptr ) ntl::km::pool<ntl::km::PagedPool>::free(ptr);
}

__forceinline
void * __cdecl
  operator new(std::size_t size, const std::nothrow_t&) __ntl_nothrow
{
  return ntl::km::pool<ntl::km::NonPagedPool>::alloc(size);
}

__forceinline
void __cdecl
  operator delete(void* ptr, const std::nothrow_t&) __ntl_nothrow
{
  if ( ptr ) ntl::km::pool<ntl::km::NonPagedPool>::free(ptr);
}


///\name  Array forms

__forceinline
void * __cdecl
  operator new[](std::size_t size)
{
  return ntl::km::pool<ntl::km::PagedPool>::alloc(size);
}

__forceinline
void __cdecl
  operator delete[](void* ptr) __ntl_nothrow
{
  if ( ptr ) ntl::km::pool<ntl::km::PagedPool>::free(ptr);
}

__forceinline
void * __cdecl
  operator new[](std::size_t size, const std::nothrow_t&) __ntl_nothrow
{
  return ntl::km::pool<ntl::km::NonPagedPool>::alloc(size);
}

__forceinline
void __cdecl
  operator delete[](void* ptr, const std::nothrow_t&) __ntl_nothrow
{
  if ( ptr ) ntl::km::pool<ntl::km::NonPagedPool>::free(ptr);
}

#endif//#ifndef NTL_NO_NEW

#endif//#ifndef NTL__KM_NEW
