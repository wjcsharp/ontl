/**\file*********************************************************************
 *                                                                     \brief
 *  NT time
 *
 ****************************************************************************
 */
#ifndef NTL__KM_TIME
#define NTL__KM_TIME
#pragma once

#include "basedef.hxx"
#include "../nt/shared_data.hxx"
#include "../nt/time.hxx"

namespace ntl {
  namespace km {

    using nt::time_fields;
    using nt::system_time;
    using nt::systime_t;
    using nt::RtlTimeToTimeFields;

    struct ktimer
    {
      dispatcher_header Header;
      uint64_t          DueTime;
      list_entry        TimerListEntry;
      kdpc *            Dpc;
      long              Period;
    };

    __forceinline
      void KeQuerySystemTime(systime_t* CurrentTime)
    {
      *CurrentTime = user_shared_data::instance().SystemTime.get();
    }

    __forceinline
      void KeQueryTickCount(int64_t* TickCount)
    {
      *TickCount = user_shared_data::instance().TickCountQuad;
    }

    NTL_EXTERNAPI
      void __stdcall
      ExSystemTimeToLocalTime(systime_t* SystemTime, systime_t* LocalTime);


  }//namespace km
}//namespace ntl

#endif//#ifndef NTL__KM_TIME
