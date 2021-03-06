/**\file*********************************************************************
 *                                                                     \brief
 *  NT Types support library
 *
 ****************************************************************************
 */
#ifndef NTL__NT_BASEDEF
#define NTL__NT_BASEDEF
#pragma once

// VC compiler always under NT and for NT (if not for KM)
#if defined _MSC_VER && !defined(NTL_SUBSYSTEM_KM)
# define NTL_SUBSYSTEM_NT
#endif
#ifndef NTL_SUBSYSTEM_NS
# define NTL_SUBSYSTEM_NS ntl::nt
#endif

#include "status.hxx"
#include "../basedef.hxx"
#include "../stdlib.hxx"


extern "C"
#ifdef _MSC_VER
__declspec(selectany)
#endif
/*C2373 volatile*/ std::uintptr_t __security_cookie = //-V101
  #ifdef _M_X64
    0x00002B992DDFA232;
  #else
    0xBB40E64E;
  #endif

#if defined(__ICL) || defined(__clang__)
struct __s_GUID
{
  unsigned long Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char Data4[8];
};
#endif
//typedef __s_GUID _GUID;


namespace ntl {

  namespace km {
    namespace priority
    {
      enum type {
        low          =  0,    // Lowest thread priority level
        low_realtime = 16,    // Lowest realtime priority level
        high         = 31,    // Highest thread priority level
        maximum      = 32     // Number of thread priority levels
      };
    }
    typedef uintptr_t kaffinity;
    typedef long      kpriority;
  }

  namespace pe {
    class image;
  }

  /// Native subsystem library
  namespace nt {

    /**\addtogroup  native_types_support *** NT Types support library ***********
    *@{*/
  //#ifdef __ICL
  //#endif
    typedef __s_GUID guid, guid_t;


    typedef const struct _opaque { } * legacy_handle;

    struct client_id
    {
      legacy_handle UniqueProcess;
      legacy_handle UniqueThread;
    };


    ///\name Native list types
    struct list_entry
    {
      union { list_entry * Flink; list_entry * next; }; //-V117
      union { list_entry * Blink; list_entry * prev; }; //-V117

      void link(list_entry * prev, list_entry * next)
      {
        this->prev = prev; this->next = next;
        prev->next = this; next->prev = this;
      }

      void unlink()
      {
        next->prev = prev;
        prev->next = next;
      }
    };


    struct list_head : public list_entry
    {
      list_head()
      {
        next = this;
        prev = this;
      }

      bool empty() const { return next == this; }

      void insert(list_entry * position, list_entry * entry)
      {
        entry->link(position, position->next);
      }

      list_entry * erase(list_entry * position)
      {
        list_entry * const next = position->next;
        position->unlink();
        return next;
      }

      void insert_tail(list_entry * entry)
      {
        entry->link(prev, this);
      }

      list_entry * begin() { return next; }
      list_entry * end()   { return this; }
      const list_entry * begin() const { return next; }
      const list_entry * end()   const { return this; }
      const list_entry * cbegin()const { return next; }
      const list_entry * cend()  const { return this; }
    };


    struct single_list_entry
    {
      single_list_entry * Next;

      single_list_entry * next() const { return Next; }
      void next(single_list_entry * p)  { Next = p; }
    };

    typedef single_list_entry slist_entry;

    struct alignas(8) slist_header : public slist_entry
    {
      uint16_t    Depth;
      uint16_t    Sequence;
    };
    ///\}


    /// Masks for the predefined standard access types
    enum access_mask: uint32_t
    {
      no_access = 0,
      delete_access             = 0x00010000L,
      read_control              = 0x00020000L,
      write_dac                 = 0x00040000L,
      write_owner               = 0x00080000L,
      synchronize               = 0x00100000L,
      standard_rights_required  = delete_access | read_control | write_dac | write_owner,
      standard_rights_read      = read_control,
      standard_rights_write     = read_control,
      standard_rights_execute   = read_control,
      standard_rights_all       = standard_rights_required | synchronize,
      specific_rights_all       = 0x0000FFFFL,
      access_system_security    = 0x01000000L,
      maximum_allowed           = 0x02000000L,
      generic_read              = 0x80000000L,
      generic_write             = 0x40000000L,
      generic_execute           = 0x20000000L,
      generic_all               = 0x10000000L
    };

    static inline
      access_mask  operator | (access_mask m, access_mask m2)
    {
      return bitwise_or(m, m2);
    }

    static inline
      access_mask  operator & (access_mask m, access_mask m2)
    {
      return bitwise_and(m, m2);
    }


    struct generic_mapping
    {
      access_mask generic_read;
      access_mask generic_write;
      access_mask generic_execute;
      access_mask generic_all;
    };


    #pragma warning(push)
    #pragma warning(disable:4201) // nameless union
    struct io_status_block
    {
      union { ntstatus  Status; void * Pointer; }; //-V117
      uintptr_t Information;
    };
    #pragma warning(pop)


    typedef
      void __stdcall
      io_apc_routine(
        const void *            ApcContext,
        const io_status_block * IoStatusBlock,
        uint32_t                Reserved
        );


    /**@} native_types_support */

    enum system_power_state
    {
      PowerSystemUnspecified,
      PowerSystemWorking,
      PowerSystemSleeping1,
      PowerSystemSleeping2,
      PowerSystemSleeping3,
      PowerSystemHibernate,
      PowerSystemShutdown,
      PowerSystemMaximum
    };


    enum power_action
    {
      PowerActionNone,
      PowerActionReserved,
      PowerActionSleep,
      PowerActionHibernate,
      PowerActionShutdown,
      PowerActionShutdownReset,
      PowerActionShutdownOff,
      PowerActionWarmEject
    };

    enum device_power_state
    {
      PowerDeviceUnspecified,
      PowerDeviceD0,
      PowerDeviceD1,
      PowerDeviceD2,
      PowerDeviceD3,
      PowerDeviceMaximum
    };

    __class_enum(DllMainReason)
    {
      DllProcessDetach,
      DllProcessAttach,
      DllThreadAttach,
      DllThreadDetach
    };};

NTL_EXTERNAPI ntstatus __stdcall ZwYieldExecution();

static inline ntstatus yield_execution()
{
  return ZwYieldExecution();
}

// system_time::type
typedef int64_t systime_t;

NTL_EXTERNAPI
ntstatus __stdcall
  NtDelayExecution(
    bool              Alertable,
    const systime_t&  DelayInterval
    );

template<times::type TimeResolution>
static inline
ntstatus sleep(
  uint32_t        time_resolution,
  bool            alertable = false)
{
  const systime_t interval = int64_t(-1) * TimeResolution * time_resolution;
  return NtDelayExecution(alertable, interval);
}

/// default milliseconds
static inline
ntstatus sleep(
  uint32_t        ms,
  bool            alertable = false)
{
  const systime_t interval = int64_t(-1) * times::milliseconds * ms;
  return NtDelayExecution(alertable, interval);
}

/// rtl compression

namespace compression
{
  enum engines
  {
    engine_standart,
    engine_maximum   = 0x0100
  };

  enum formats
  {
    format_none,
    format_default,
    format_lznt1
  };

  //inline uint16_t operator|(formats f, engines e)
  //{
  //  return static_cast<uint16_t>( uint16_t(f) | uint16_t(e) );
  //}

  static const uint16_t default_format = engine_maximum|format_lznt1;

} // compression

NTL_EXTERNAPI
ntstatus __stdcall
  RtlGetCompressionWorkSpaceSize (
    uint16_t CompressionFormatAndEngine,
    uint32_t* CompressBufferWorkSpaceSize,
    uint32_t* CompressFragmentWorkSpaceSize
    );

NTL_EXTERNAPI
ntstatus __stdcall
  RtlCompressBuffer (
    uint16_t CompressionFormatAndEngine,
    const void* UncompressedBuffer,
    uint32_t UncompressedBufferSize,
    void* CompressedBuffer,
    uint32_t CompressedBufferSize,
    uint32_t UncompressedChunkSize,
    uint32_t* FinalCompressedSize,
    void* WorkSpace
    );

NTL_EXTERNAPI
ntstatus __stdcall
  RtlDecompressFragment (
    uint16_t CompressionFormat,
    void* UncompressedFragment,
    uint32_t UncompressedFragmentSize,
    const void* CompressedBuffer,
    uint32_t CompressedBufferSize,
    uint32_t FragmentOffset,
    uint32_t* FinalUncompressedSize,
    void* WorkSpace
    );

NTL_EXTERNAPI
ntstatus __stdcall
  RtlDecompressBuffer (
    uint16_t CompressionFormat,
    void* UncompressedBuffer,
    uint32_t UncompressedBufferSize,
    const void* CompressedBuffer,
    uint32_t CompressedBufferSize,
    uint32_t* FinalUncompressedSize
    );

NTL_EXTERNAPI
  uint32_t __stdcall RtlUniform(uint32_t* Seed);

NTL_EXTERNAPI
  uint32_t __stdcall RtlRandom(uint32_t* Seed);

NTL_EXTERNAPI
  uint32_t __stdcall RtlRandomEx(uint32_t* Seed);

namespace harderror
{
  namespace response
  {
    enum type {
      ReturnToCaller,
      NotHandled,
      Abort,
      Cancel,
      Ignore,
      No,
      Ok,
      Retry,
      Yes
    };
  }

  namespace option
  {
    enum type {
      AbortRetryIgnore,
      Ok,
      OkCancel,
      RetryCancel,
      YesNo,
      YesNoCancel,
      ShutdownSystem
    };
  }

  typedef response::type responses;
  typedef option::type   options;

  typedef ntstatus __stdcall raise_hard_error_t(
    ntstatus    ErrorStatus,
    uint32_t    NumberOfParameters,
    uint32_t    UnicodeStringParameterMask,
    uintptr_t*  Parameters,
    options     ValidResponseOptions,
    responses&  Response
    );

} // harderror

NTL_EXTERNAPI harderror::raise_hard_error_t NtRaiseHardError;

  }//namespace nt
}//namespace ntl

#endif//#ifndef NTL__NT_BASEDEF
