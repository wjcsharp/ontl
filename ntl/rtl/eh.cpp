////////////////////////////////////////////////////////////////////////////////
///
///\file
///\brief C++ exception support runtime
///
///\note  The file contains compiler predefined library helpers
///       and must be compiled without /GL to avoid C2268.
///       So it is the separate compilation unit to support /GL for other ones.
///
////////////////////////////////////////////////////////////////////////////////

#define NTL_EH_RUNTIME

#ifdef _M_X64
//# define NTL__OTHEREHMAGICS
#include "../pe/image.hxx"
#endif

#include "../atomic.hxx"
#include "../stlx/cstdlib.hxx"
#include "../nt/exception.hxx"
#include "../nt/context.hxx"
#include "../nt/status.hxx"

namespace dbg = ntl::nt::dbg;

#ifdef NTL_DEBUG
# include "../nt/debug.hxx"
  void dbg_pause()
  {
    ntl::nt::dbg::bp();
  }
#endif

#pragma region std::unexpected
/// RTL poiner to the current unexpected_handler
volatile std::unexpected_handler __ntl_std_unexpected_handler;

std::unexpected_handler std::get_unexpected() __ntl_nothrow
{
  return __ntl_std_unexpected_handler;
};

std::unexpected_handler std::set_unexpected(std::unexpected_handler f) __ntl_nothrow
{
  assert(f != nullptr);
  unexpected_handler old = f;
  return ntl::atomic::generic_op::exchange(__ntl_std_unexpected_handler, old);
}

void std::unexpected()
{
  std::unexpected_handler handler = __ntl_std_unexpected_handler;
  if(handler)
    __ntl_std_unexpected_handler();
  else{
    __debugbreak();
    std::terminate();
  }
}

bool std::uncaught_exception() __ntl_nothrow
{
  using namespace ntl::cxxruntime;
   tiddata* ptd = _getptd_noinit();
   return ptd && ptd->nestedExcount > 0;
};

#pragma endregion

using namespace ntl;
using namespace ntl::cxxruntime;

#ifdef _M_IX86
static __forceinline
uint32_t get_eax()
{
  __asm xchg eax, eax
}

extern "C" void __cdecl _chkstk();
extern "C" __declspec(naked) void _alloca_probe_16()
{
  __asm {
    push ecx
    lea  ecx,[esp+8]
    sub  ecx,eax
    And  ecx,15
    add  eax,ecx
    sbb  ecx,ecx
    Or   eax,ecx
    pop  ecx
    jmp  _chkstk
  }
}

#pragma warning (push)
// This library helper shall receive only valid handlers
#pragma warning (disable:4733)//Inline asm assigning to 'FS:0' : handler not registered as safe handler
extern "C" __declspec(naked)
void __cdecl _EH_prolog(void* /*handler*/)
{
  __asm 
  {
    push  -1
    push  eax
    mov   eax, fs:0
    push  eax
    mov   eax, [esp+3*4]
    mov   fs:0, esp
    mov   [esp+3*4], ebp
    lea   ebp, [esp+3*4]
    push  eax
    retn
  }
}
#pragma warning (pop)
#endif

#ifdef _M_X64


exception_disposition
ntl::cxxruntime::cxxframehandler(
                exception_record *        const er,       ///< thrown NT exception
                cxxregistration *         const eframe,
                const nt::context *       const ectx,
                dispatcher_context *      const dispatch,
                const ehfuncinfo *              const ehfi,     ///< registered handlers
                int                             const trylevel      /* CatchDepth */,
                const exception_registration *  const nested_eframe /* pMarkerRN */,
                bool                            const destruct      /* recursive */)
{
  if(!
    (er->iscxx()
    || (er->isconsolidate() && er->NumberParameters == exception_record::maximum_parameters && er->ExceptionInformation[8] == ehmagic1200)
    || er->islongjump() || ehfi->magic < _EH_MAGIC || !ehfi->synchronous)
    )
  {
    // TODO: FindHandlerForForeignException
    return ExceptionContinueSearch;
  }

  // check a call by RtlUnwind
  if(er->ExceptionFlags & nt::exception::unwind){
    if(ehfi->unwindtable_size && trylevel == 0) {
      bool unwinded = false;
      if(er->ExceptionFlags & nt::exception::target_unwind){
        if(er->islongjump()){
          __debugbreak();
          ehstate_t state = ehfi->state_from_ip(dispatch, (void*)ectx->Rip);
          if(state < -1 || state >= ehfi->unwindtable_size)
            ntl::nt::exception::inconsistency();
          // __FrameUnwindToState
          eframe->unwind(dispatch, ehfi, state);
          unwinded = true;
        }else if(er->isconsolidate()){
          ehstate_t state = static_cast<ehstate_t>(er->ExceptionInformation[3]);
          assert(state >= -1 && state < ehfi->unwindtable_size);
          if(state < -1 || state >= ehfi->unwindtable_size)
            nt::exception::inconsistency();
          cxxregistration* frame = reinterpret_cast<cxxregistration*>(er->ExceptionInformation[1]);
          frame->unwind(dispatch, ehfi, state);
          unwinded = true;
        }
      }
      if(!unwinded){
        // __FrameUnwindToEmptyState
        cxxregistration::frame_pointers frame;
        cxxregistration* unwindframe = eframe->establisherframe(ehfi, dispatch, &frame);
        ehstate_t state = ehfi->state_from_control(dispatch);
        assert(state >= -1 && state < ehfi->unwindtable_size);
        const tryblock* tb = ehfi->catch_try_block(state);
        ehstate_t to_state = tb ? tb->tryhigh : -1;
        unwindframe->unwind(dispatch, ehfi, to_state);
      }
    }
    // async exceptions are to be unwinded by __except_handler3
    return ExceptionContinueSearch;
  }

  if(!ehfi->tryblocktable_size){
    if(ehfi->magic < ehmagic1300 || !ehfi->unwindhelp)
      return ExceptionContinueSearch;
  }

  cxxrecord* cxxer = static_cast<cxxrecord*>(er);
  if(cxxer->iscxx() && er->NumberParameters >= 3 && cxxer->get_ehmagic() > ehmagic1400){
    throwinfo::forwardcompathandler_t* fch = NULL;
    if(cxxer->get_throwinfo()->forwardcompathandler){
      fch = (throwinfo::forwardcompathandler_t*)(cxxer->get_throwinfo()->forwardcompathandler + cxxer->get_throwimagebase());
      return fch(er, eframe, ectx, dispatch, ehfi, trylevel, nested_eframe, destruct);
    }
  }

  // not cxx ?
  cxxer->find_handler(eframe, ectx, dispatch, ehfi, destruct, trylevel, nested_eframe);

  return ExceptionContinueSearch;
}

extern "C" generic_function_t* RtlVirtualUnwind(uint32_t HandlerType, uintptr_t ImageBase, uintptr_t ControlPc, nt::exception::runtime_function* FunctionEntry, nt::context* Context, 
                                                void** HandlerData, uintptr_t* EstablisherFrame, void* ContextPointers);


extern "C" void* _GetImageBase()
{
  __debugbreak();
  return pe::image::this_module();
}
void cxxregistration::unwindnestedframes(const exception_record* ehrec, const nt::context* ctx, cxxregistration* establishedframe, const void* handler, int state, const ehfuncinfo* ehfi, dispatcher_context* const dispatch, bool recursive)
{
  static const exception_record ehtemplate = {ntl::nt::status::unwind_consolidate, exception_noncontinuable, 0, 0, exception_record::maximum_parameters, {ehmagic1200}};

  exception_record er = ehtemplate;
  er.ExceptionInformation[0] = (uintptr_t)CxxCallCatchBlock,
    er.ExceptionInformation[1] = (uintptr_t)establishedframe,
    er.ExceptionInformation[2] = (uintptr_t)handler,
    er.ExceptionInformation[3] = state,
    er.ExceptionInformation[4] = (uintptr_t)ctx,
    er.ExceptionInformation[5] = (uintptr_t)ehfi,
    er.ExceptionInformation[6] = (uintptr_t)ehrec,
    er.ExceptionInformation[7] = recursive,
    er.ExceptionInformation[8] = ehmagic1200;

  exception_pointers ep; ep.ExceptionRecord = reinterpret_cast<exception_record*>(er.ExceptionInformation[6]);
  assert(!ep.ExceptionRecord || ep.ExceptionRecord->NumberParameters <= exception_record::maximum_parameters);

  //tiddata* ptd = _getptd();
  //dbg::trace.printf("sending   ehrec %p, cur ex %p (valid: %d)\n", ep.ExceptionRecord, ptd->curexception.ExceptionRecord, !ptd->curexception.ExceptionRecord || ptd->curexception.ExceptionRecord->NumberParameters <= 15);
  nt::context octx;
  RtlUnwindEx(fp.FramePointers, dispatch->ControlPc, &er, 0, &octx, dispatch->HistoryTable);
}

exception_filter ExFilterRethrow(exception_pointers* ep, cxxrecord* old, bool& rethrown)
{
  //__debugbreak();
  cxxrecord* ehrec = static_cast<cxxrecord*>(ep->ExceptionRecord);
  rethrown = false;
  if(ehrec->is_msvc(true)){
    if(ehrec->ExceptionInformation[1] == old->ExceptionInformation[1])
      rethrown = true;
    if(ehrec->ExceptionInformation[1] == 0){
      _getptd()->processingThrow = 1;
      rethrown = true;
      return exception_execute_handler;
    }
  }
  return exception_continue_search;
}

void RethrowException(exception_record* ehrec)
{
  cxxrecord* cxxer = static_cast<cxxrecord*>(ehrec);
  cxxer->raise();
}

extern "C" generic_function_t* ntl::cxxruntime::CxxCallCatchBlock(exception_record* ehrec)
{
  // save the current exception
  tiddata* ptd = _getptd();
  exception_pointers* ep = &ptd->curexception;//&_getptd()->curexception;
  exception_pointers saved_exception = *ep;
  assert(!ep->ExceptionRecord || ep->ExceptionRecord->NumberParameters <= exception_record::maximum_parameters);
  
  ep->ExceptionRecord = reinterpret_cast<exception_record*>(ehrec->ExceptionInformation[6]);
  assert(ep->ExceptionRecord->NumberParameters <= exception_record::maximum_parameters);
  //dbg::trace.printf("received  ehrec %p, cur ex %p (valid: %d)\n", ep->ExceptionRecord, ptd->curexception.ExceptionRecord, !ptd->curexception.ExceptionRecord || ptd->curexception.ExceptionRecord->NumberParameters <= 15);
  ep->ContextRecord = reinterpret_cast<nt::context*>(ehrec->ExceptionInformation[4]);
  _getptd()->nestedExcount++;
  assert(!ep->ExceptionRecord || ep->ExceptionRecord->NumberParameters <= exception_record::maximum_parameters);

  // original record
   ehfuncinfo* ehfi = reinterpret_cast<ehfuncinfo*>(ehrec->ExceptionInformation[5]);
  cxxrecord* cxxer = reinterpret_cast<cxxrecord*>(ehrec->ExceptionInformation[6]);
  cxxregistration* eframe = reinterpret_cast<cxxregistration*>(ehrec->ExceptionInformation[1]);
  cxxregistration::frame_info frame(cxxer->get_object());
  generic_function_t* ret = 0;
  bool rethrow = false;

  exception_record* tmpER = 0;
  generic_function_t *handler = reinterpret_cast<generic_function_t*>(ehrec->ExceptionInformation[2]);
  bool translated = ehrec->ExceptionInformation[7] != 0;
  if(translated){
    tmpER = _getptd()->prevER;
    assert(!tmpER || tmpER->NumberParameters <= exception_record::maximum_parameters);
    ep->ExceptionRecord = tmpER;
  }

  __try{
    __try{
      ret = _CallSettingFrame(handler, eframe, 0x100);
    }
    __except(ExFilterRethrow(ntl::_exception_info(), cxxer, rethrow)){
      rethrow = true;
      _getptd()->processingThrow = 0;
      if(translated){
        cxxer->destruct_eobject(true);
        RethrowException(_getptd()->prevER);
      }else{
        RethrowException(cxxer);
      }
    }

  }
  __finally{
    frame.unlink();
    //rethrow |= _abnormal_termination() != 0;
    if(!rethrow && cxxer->is_msvc(true)){
      if(cxxregistration::frame_info::find(cxxer->get_object()))
        cxxer->destruct_eobject(!!_abnormal_termination());
    }
    _getptd()->nestedExcount--;
    // restore saved exception
    ep = &_getptd()->curexception;
    //exception_record* old = ep->ExceptionRecord;
    *ep = saved_exception;
    //dbg::trace.printf("restored cur ex %p (valid: %d) (was %p)\n", ptd->curexception.ExceptionRecord, !ptd->curexception.ExceptionRecord || ptd->curexception.ExceptionRecord->NumberParameters <= 15, old);
    assert(!ep->ExceptionRecord || ep->ExceptionRecord->NumberParameters <= exception_record::maximum_parameters);
  }

  *reinterpret_cast<intptr_t*>(eframe->fp.FramePointers + ehfi->unwindhelp) = -2;
  return ret;
}

#endif // _M_X64

/// throw T; implementation.
/// 15.1/1  Throwing an exception transfers control to a handler
///         An object is passed and the type of that object determines which
///         handlers can catch it.
///\note    MSVC's `throw;` statement sets both pointers to nulls.
#if defined(__ICL)// && defined(_M_IX86)
typedef ntl::cxxruntime::throwinfo _s__ThrowInfo;
extern "C"
__noreturn
#else
__noreturn
extern "C"
#endif
void __stdcall _CxxThrowException(void * object, _s__ThrowInfo const * info)
{
#ifdef _M_IX86
  std::array<uintptr_t, 3> args = { _EH_MAGIC, (uintptr_t)object, (uintptr_t)info };
#if _MSC_VER >= 1700
  const throwinfo* ti = reinterpret_cast<const throwinfo*>(info);
  if(info && ti->e8)
    args[0] = ehmagic1994;
#endif
#endif // _M_IX86
#ifdef _M_X64
  void* imagebase;
  ntl::nt::RtlPcToFileHeader(info, &imagebase);
  std::array<uintptr_t, 4> args = { _EH_MAGIC, (uintptr_t)object, (uintptr_t)info, (uintptr_t)imagebase};
  const throwinfo* ti = reinterpret_cast<const throwinfo*>(info);
  if(info && (ti->e8 || !imagebase))
    args[0] = ehmagic1994;
#endif // _M_X64
  raise_exception(exception_record::cxxmagic, nt::exception::noncontinuable, args);
}

template<typename T>
void fixptr(uintptr_t base, const rva_t ptr, T& vptr)
{
  vptr = (T)(base + ptr);
}

///\see exception_handler
/// indirectly called by _CxxThrowException -> RtlDispatchException (RtlpExecuteHandlerForException)
/// OR RtlUnwind (RtlpExecuteHandlerForUnwind)
extern "C"
exception_disposition
__cdecl ///\note actually the first arg is passed in EAX register on x86
__CxxFrameHandler3(
  /** ehfuncinfo * eax on x86 */
  exception_record *    er,       ///< thrown NT exception
  cxxregistration *     frame,    ///< tib::ExceptionList node
  nt::context *         ectx,     ///< not used on x86
  dispatcher_context *  dispatch  ///< not used on x86
  )
{
#ifdef _M_IX86
  const ehfuncinfo* /*const*/ ehfi = reinterpret_cast<const ehfuncinfo*>(get_eax());
  return cxxframehandler(er, frame, ectx, dispatch, ehfi);
#endif
#ifdef _M_X64
  uintptr_t imagebase;
  RtlPcToFileHeader(__CxxFrameHandler3, (void**)&imagebase);

  const ehfuncinfo_packed* ehfip = reinterpret_cast<const ehfuncinfo_packed*>( imagebase + *dispatch->HandlerData );

#if 0
  struct frame_t
  {
    cxxregistration* frame;
    uintptr_t var30;
    uintptr_t var38;
    uintptr_t var40;
  } lframe = {frame};
#else
  cxxregistration* lframe = frame;
#endif

  ehfuncinfo efi(*ehfip, imagebase);
  const ehfuncinfo* ehfi = &efi;

#if 0
  // check current states
  const cxxrecord* cxxer = static_cast<cxxrecord*>(er);
  if(cxxer->get_throwinfo()){
    cxxregistration* fp = ntl::padd(frame, ehfi->unwindhelp);
    assert(fp->fp.MemoryStackFp >= -2 && fp->fp.MemoryStackFp < ehfi->unwindtable_size);
    assert(fp->fp.MemoryStoreFp >= -2 && fp->fp.MemoryStoreFp < ehfi->unwindtable_size);
  }
#endif
  return cxxframehandler(er, reinterpret_cast<cxxregistration*>(&lframe), ectx, dispatch, ehfi);
#endif
}

extern "C" exception_disposition __cdecl __CxxFrameHandler(exception_record* er, cxxregistration* frame, nt::context* ectx, dispatcher_context* dispatch)
{
#ifdef _M_IX86
  const ehfuncinfo* /*const*/ ehfi = reinterpret_cast<const ehfuncinfo*>(get_eax());
  return cxxframehandler(er, frame, ectx, dispatch, ehfi);
#else
  return __CxxFrameHandler3(er,frame,ectx,dispatch);
#endif
}

///\todo __EH_prolog for /Os

void ntl::nt::exception::inconsistency()
{
  __debugbreak();
  std::abort();
}


///\name EH support for array of objects
#ifdef _M_X64
# define __ehcall __stdcall
#else
# define __ehcall __thiscall
#endif

#if 0

struct UDT {};
typedef void (UDT::* xtor)();
#define CALL_XTOR(callee, ptr) ((reinterpret_cast<UDT*>(ptr)->*(callee))())

#else

typedef void(__thiscall* xtor)(void*);
#define CALL_XTOR(callee, ptr) ( (callee)(ptr) )

#endif

#if _MSC_VER >= 1900
  typedef size_t ehvec_count;
#else
  typedef int ehvec_count;
#endif

static int array_unwind_filter(exception_pointers* eh)
{
  switch(eh->ExceptionRecord->ExceptionCode)
  {
  case exception_record::cxxmagic:
    std::terminate();
    break;
  }
  return exception_continue_search;
}

/** unwind corrupted array */
static void array_unwind(void* ptr, size_t size, ehvec_count count, xtor dtor)
{
  __try{
    while(--count >= 0){
      ptr = ntl::padd(ptr, -static_cast<std::ssize_t>(size));
      CALL_XTOR(dtor, ptr);
    }
  }__except(array_unwind_filter(exception_info())){

  }
}


// Destructor support for arrays of objects
void __stdcall __ehvec_dtor(void* ptr, size_t size, ehvec_count count, xtor dtor)
{
  __try{
    // destroy from end to begin
    ptr = ntl::padd(ptr, size*count);
    int ssize = -static_cast<int>(size);
    while(--count >= 0){
      ptr = ntl::padd(ptr, ssize);
      CALL_XTOR(dtor, ptr);
    }
  }
  __except(array_unwind_filter(exception_info())){
  }
}

/** Constructor */
void __stdcall __ehvec_ctor(void* ptr, size_t size, ehvec_count count, xtor ctor, xtor dtor)
{
  ehvec_count i = 0;
  bool success = false;
  __try{
    for(i = 0; i < count; i++){
      CALL_XTOR(ctor, ptr);
      ptr = ntl::padd(ptr, size);
    }
    success = true;
  }
  __finally{
    if(!success)
      array_unwind(ptr, size, i, dtor);
  }
}
