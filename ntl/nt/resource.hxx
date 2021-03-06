/**\file*********************************************************************
 *                                                                     \brief
 *  Shared resource support
 *
 ****************************************************************************
 */
#ifndef NTL__NT_RESOURCE
#define NTL__NT_RESOURCE
#pragma once

#include "handle.hxx"
#include "../stlx/chrono.hxx"

namespace ntl {
  namespace nt {

    namespace rtl
    {
      // critical section
      struct critical_section_debug
      {
        enum type { CritSect, Resource };
        uint16_t    Type;
        uint16_t    CreatorBackTraceIndex;
        struct critical_section* CriticalSection;
        list_entry  ProcessLocksList;
        uint32_t    EntryCount;
        uint32_t    ContentionCount;
        uint32_t    Spare[2];
      };

      struct critical_section
      {
        critical_section_debug* DebugInfo;
        int32_t       LockCount;
        int32_t       RecursionCount;
        legacy_handle OwningThread;
        legacy_handle LockSemaphore;
        uint32_t      reserved;
      };

      struct resource_debug
      {
        uint32_t      reserved[5];
        uint32_t      ContentionCount;
        uint32_t      Spare[2];
      };

      // shared resource
      struct resource
      {
        critical_section  CriticalSection;
        legacy_handle     SharedSemaphore;
        uint32_t          NumberOfWaitingShared;
        legacy_handle     ExclusiveSemaphore;
        uint32_t          NumberOfWaitingExclusive;

        int32_t           NumberOfActive; // negative: exclusive acquire; zero: not acquired; positive: shared acquire(s)
        legacy_handle     ExclusiveOwnerThread;

        enum flags { None, LongTerm };
        flags             Flags;
        resource_debug*   DebugInfo;
      };

      // run once
      union run_once
      {
        void* Ptr;
      };

      static const uint32_t RunOnceCheckOnly = 1U;
      static const uint32_t RunOnceAsync     = 2U;

      typedef uint32_t __stdcall run_once_init_t(
          rtl::run_once* RunOnce,
          void* Parameter,
          void** Context
          );

    } // rtl


    typedef ntstatus __stdcall critical_section_control_t(rtl::critical_section* CriticalSection);

    NTL_EXTERNAPI
      critical_section_control_t
        RtlInitializeCriticalSection,
        RtlDeleteCriticalSection,
        RtlEnterCriticalSection,
        RtlLeaveCriticalSection;

    NTL_EXTERNAPI
      ntstatus __stdcall RtlInitializeCriticalSectionAndSpinCount(
        rtl::critical_section* CriticalSection,
        uint32_t               SpinCount
        );

    NTL_EXTERNAPI
      uint32_t __stdcall RtlTryEnterCriticalSection(rtl::critical_section* CriticalSection);

    /** These begins with WS2003SP1 
    NTL_EXTERNAPI
      uint32_t __stdcall RtlIsCriticalSectionLocked(rtl::critical_section* CriticalSection);

    NTL_EXTERNAPI
      uint32_t __stdcall RtlIsCriticalSectionLockedByThread(rtl::critical_section* CriticalSection);
    */

    NTL_EXTERNAPI
      uint32_t __stdcall RtlGetCriticalSectionRecursionCount(const rtl::critical_section* CriticalSection);

    NTL_EXTERNAPI
      uint32_t __stdcall RtlSetCriticalSectionSpinCount(rtl::critical_section* CriticalSection, uint32_t SpinCount);

    NTL_EXTERNAPI
      void __stdcall RtlEnableEarlyCriticalSectionEventCreation();

    NTL_EXTERNAPI
      void __stdcall RtlCheckForOrphanedCriticalSections(legacy_handle Thread);

    NTL_EXTERNAPI
      void __stdcall RtlpWaitForCriticalSection(rtl::critical_section* CriticalSection);

    NTL_EXTERNAPI
      void __stdcall RtlpUnWaitCriticalSection(rtl::critical_section* CriticalSection);

    NTL_EXTERNAPI
      void __stdcall RtlpNotOwnerCriticalSection(rtl::critical_section* CriticalSection);

    // run once
    NTL_EXTERNAPI
      void RtlRunOnceInitialize(rtl::run_once* RunOnce);

    NTL_EXTERNAPI
      uint32_t RtlRunOnceExecuteOnce(
        rtl::run_once*        RunOnce,
        rtl::run_once_init_t  InitFn,
        void*                 Parameter,
        void**                Context
      );

    NTL_EXTERNAPI
      uint32_t RtlRunOnceBeginInitialize(
        rtl::run_once*        RunOnce,
        uint32_t              Flags,
        void**                Context
      );

    NTL_EXTERNAPI
      uint32_t RtlRunOnceComplete(
        rtl::run_once*        RunOnce,
        uint32_t              Flags,
        void**                Context
      );

    //////////////////////////////////////////////////////////////////////////

    typedef void __stdcall resource_control_t(rtl::resource*);
   
    NTL_EXTERNAPI 
      resource_control_t
      RtlInitializeResource,
      RtlReleaseResource,
      RtlDeleteResource,
      RtlConvertSharedToExclusive,
      RtlConvertExclusiveToShared;
 
    NTL_EXTERNAPI 
    bool __stdcall
      RtlAcquireResourceShared(rtl::resource* Resource, bool Wait);

    NTL_EXTERNAPI 
    bool __stdcall
      RtlAcquireResourceExclusive(rtl::resource* Resource, bool Wait);


    /************************************************************************/
    /* CS RAII                                                              */
    /************************************************************************/

    /**
    *	Critical section RAII wrapper
    **/
    class critical_section:
      public rtl::critical_section,
      ntl::noncopyable
    {
    public:
      class guard;

      /** Constructs CS object */
      critical_section()
      {
        ntl::nt::RtlInitializeCriticalSection(this);
      }

      /** Constructs CS object with the specified spin count */
      explicit critical_section(uint32_t SpinCount)
      {
        ntl::nt::RtlInitializeCriticalSectionAndSpinCount(this, SpinCount);
      }

      /** Destroys CS object. If CS was owned, it is undefined behaviour. */
      ~critical_section()
      {
        ntl::nt::RtlDeleteCriticalSection(this);
      }
 
      /** Waits and takes ownership of CS. */
      void acquire()
      {
        ntl::nt::RtlEnterCriticalSection(this);
      }
 
      /** Releases owlership. */
      void release()
      {
        ntl::nt::RtlLeaveCriticalSection(this);
      }
 
      /** Attempts to take ownership of this CS object. */
      bool try_acquire()
      {
        return ntl::nt::RtlTryEnterCriticalSection(this) != 0;
      }
 
      /** Checks owlership of this CS. */
      bool locked() const 
      {
        return (LockCount & 1) == 0;
        //return ntl::nt::RtlIsCriticalSectionLocked(this) != 0;
      }
 
      /** Checks is the current thread is owner of CS. */
      bool is_owner() const
      {
        return OwningThread == teb::instance().ClientId.UniqueThread;
        //return ntl::nt::RtlIsCriticalSectionLockedByThread(this) != 0;
      }
 
      
      /**
       *	@brief Waits the specified time for ownership of the CS
       *
       * This function implements the waiting on CS object. 
       * If the CS is haves a synchronization object, function calls standard wait mechanism,
       * otherwise it waits with a delayed execution.
       *
       *	@param[in] timeout absolute or relative time to wait.
       *	@param[in] explicit_wait If CS doesn't have a synchronization object and CS object is owled by other thread, immediately returns.
       *	@param[in] alertable waiting is alertable.
       *
       *	@return Wait status
       *
       **/
      ntstatus wait(const systime_t& timeout, bool explicit_wait, bool alertable = true)
      {
        if(LockSemaphore){
          DebugInfo->EntryCount++;
          DebugInfo->ContentionCount++;
          return NtWaitForSingleObject(LockSemaphore, alertable, timeout);
        }
 
        if(!try_acquire() && !explicit_wait)
          return status::invalid_handle;
 
        // wait
        ntstatus st = try_acquire() ? status::success : status::timeout;
        systime_t const interval = -1i64*std::chrono::duration_cast<system_duration>( std::chrono::milliseconds(50)).count();
        if(timeout < 0){
          // relative time
          systime_t const end_time = user_shared_data::instance().InterruptTime.get() + (-timeout);
          do{
            NtDelayExecution(alertable, interval);
            st = try_acquire() ? status::success : status::timeout;
          }while(st == status::timeout && user_shared_data::instance().InterruptTime.get() < end_time);

        }else if(timeout > 0){
          // absolute time
          systime_t const end_time = timeout;
          do {
            NtDelayExecution(alertable, interval);
            st = try_acquire() ? status::success : status::timeout;
          }while(st == status::timeout && query_system_time() < end_time);
        }
        return st;
      }
 
      /**
       *	@brief Waits to ownership until the specified time is occurs
       *
       *	@param[in] abs_time absolute time to wait
       *	@param[in] explicit_wait force wait if CS doesn't have synchronization object
       *	@param[in] alertable is waiting is alertable
       *
       *	@return Wait status
       *
       **/
      template <class Clock, class Duration>
      ntstatus wait_until(const std::chrono::time_point<Clock, Duration>& abs_time, bool explicit_wait = true, bool alertable = true)
      {
        return wait(std::chrono::duration_cast<system_duration>(abs_time.time_since_epoch()).count(), explicit_wait, alertable);
      }
 
      /**
       *	@brief Waits to ownership for specified time
       *
       *	@param[in] rel_time relative time to wait
       *	@param[in] explicit_wait force wait if CS doesn't have synchronization object
       *	@param[in] alertable is waiting is alertable
       *
       *	@return Wait status
       *
       **/
      template <class Rep, class Period>
      ntstatus wait_for(const std::chrono::duration<Rep, Period>& rel_time, bool explicit_wait = true, bool alertable = true)
      {
        return wait(-1i64*std::chrono::duration_cast<system_duration>(rel_time).count(), explicit_wait, alertable);
      }
 
      /** Sets spin count */
      void spin_count(uint32_t SpinCount)
      {
        ntl::nt::RtlSetCriticalSectionSpinCount(this, SpinCount);
      }
    };

    class critical_section::guard
    {
      guard(const guard&) __deleted;
      guard& operator=(const guard&) __deleted;

      critical_section& m;
    public:
      explicit guard(critical_section& m)
        : m(m)
      {
        m.acquire();
      }
      explicit guard(rtl::critical_section& cs)
        : m(static_cast<critical_section&>(cs))
      {
        m.acquire();
      }
      ~guard()
      {
        m.release();
      }
    };


    /************************************************************************/
    /* Resource RAII                                                        */
    /************************************************************************/
    class resource:
      protected rtl::resource
    {
    public:
      resource()
      {
        RtlInitializeResource(this);
      }

      ~resource()
      {
        RtlDeleteResource(this);
      }

      bool acquire(bool exclusive, bool wait)
      {
        return (exclusive ? RtlAcquireResourceExclusive : RtlAcquireResourceShared)(this, wait);
      }

      bool acquire_shared(bool wait = true)
      {
        return acquire(false, wait);
      }

      bool acquire_exclusive(bool wait = true)
      {
        return acquire(true, wait);
      }

      void release()
      {
        RtlReleaseResource(this);
      }

      void convert(bool exclusive)
      {
        (exclusive ? RtlConvertSharedToExclusive : RtlConvertExclusiveToShared)(this);
      }
    };
    
  } //namespace nt
} //namespace ntl

#endif //#ifndef NTL__NT_RESOURCE

