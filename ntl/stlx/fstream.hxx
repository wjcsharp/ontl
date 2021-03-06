/**\file*********************************************************************
 *                                                                     \brief
 *  27.9 File streams [file.streams] (N2960)
 *
 ****************************************************************************
 */
#ifndef NTL__STLX_FSTREAM
#define NTL__STLX_FSTREAM
#pragma once

#include "streambuf.hxx"
#include "istream.hxx"

#ifdef NTL_SUBSYSTEM_KM
# include "../km/file.hxx"
#else
# include "../nt/file.hxx"
#endif

#include "ext/tr2/files.hxx"

#ifndef STLX__CONFORMING_FSTREAM
// turn off UCS-MBCS character conversion in file streams
#define STLX__CONFORMING_FSTREAM 0
#endif

namespace std {

  namespace Encoding 
  {
    enum type {
      Invalid = -2,
      Default = 0, // based on bom or char type
      Ansi  = 1,
      Utf16 = 2,
      //Utf32 = 4, // unsupported
      //Utf8  = 5, // unsupported yet
    };
  }
  typedef Encoding::type EncodingType;
  namespace __fs = tr2::files;

  /**\addtogroup  lib_input_output ******* 27 Input/output library [input.output]
   *@{*/
  /**\addtogroup  lib_file_streams ******* 27.9 File-based streams [file.streams]
   *@{*/

/**
 *	@brief 27.9.1.1 Class template basic_filebuf [filebuf]
 *  @details The class basic_filebuf<charT,traits> associates both the input sequence and the output sequence with a file.
 **/
  template <class charT, class traits /*= char_traits<charT>*/ >
  class basic_filebuf:
    public basic_streambuf<charT, traits>
  {
#ifdef NTL_SUBSYSTEM_KM
    /** 16 KB default buffer size */
    static const streamsize default_file_buffer_size = 1024 * 16;
#else
    /** 1 MB default buffer size */
    static const streamsize default_file_buffer_size = 1024 * 1024;
#endif
    typedef typename traits::state_type state_type;

    basic_filebuf(const basic_filebuf& rhs) __deleted;
    basic_filebuf& operator=(const basic_filebuf& rhs) __deleted;
  public:
    typedef charT                     char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits                    traits_type;

    ///\name 27.9.1.2 basic_filebuf constructors [filebuf.cons]

    basic_filebuf()
      :mode(), our_buffer(true), encoding(Encoding::Default)
    {}

#ifdef NTL_CXX_RV
    basic_filebuf(basic_filebuf&& rhs)
      :mode(), our_buffer(true), encoding(Encoding::Default)
    {
      swap(rhs);
    }
#endif

    virtual ~basic_filebuf()
    {
      __ntl_try { 
        close();
      }
      __ntl_catch(...){}
      if(buf.second && our_buffer) // prevent to call the child virtual function
        reallocate_buffer(0);
    }

    ///\name 27.9.1.3 Assign and swap [filebuf.assign]
#ifdef NTL_CXX_RV
    basic_filebuf& operator=(basic_filebuf&& rhs)
    {
      close();
      swap(rhs);
      return *this;
    }
#endif

    void swap(basic_filebuf& rhs)
    {
      using std::swap;
      basic_streambuf::swap(rhs);
      swap(f, rhs.f);
      swap(buf, rhs.buf);
      swap(encoding, rhs.encoding);
      swap(mode, rhs.mode);
      swap(our_buffer, rhs.our_buffer);
    }

    ///\name 27.9.1.4 Member functions [filebuf.members]
    bool is_open() const { return !!f; }

    basic_filebuf<charT,traits>* open(const string& s, ios_base::openmode mode, EncodingType encoding = Encoding::Ansi)
    {
      return do_open(__fs::path(s), mode, encoding) ? this : nullptr;
    }
    basic_filebuf<charT,traits>* open(const char* s, ios_base::openmode mode, EncodingType encoding = Encoding::Ansi)
    {
      return do_open(__fs::path(s), mode, encoding) ? this : nullptr;
    }
    // NTL extension
    basic_filebuf<charT,traits>* open(const __fs::path& name, ios_base::openmode mode, EncodingType encoding = Encoding::Ansi)
    {
      return do_open(name, mode, encoding) ? this : nullptr;
    }

    basic_filebuf<charT,traits>* close()
    {
      if(!f)
        return nullptr;

      // flush
      bool ok = true;
      if(pbase()){
        const int_type eof = traits_type::eof();
        ok = !traits_type::eq_int_type(overflow(eof), eof);
      }

      /* ???
      If the last virtual member function called on *this (between underflow, overflow,
      seekoff, and seekpos) was overflow then calls a_codecvt.unshift (possibly several times) to de-
      termine a termination sequence, inserts those characters and calls overflow(traits::eof()) again.
      */

      // at last, close file
      f.close();
      return ok ? this : nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////
  protected:

    ///\name 27.9.1.5 Overridden virtual functions [filebuf.virtuals]
    virtual streamsize showmanyc()
    {
      if(!f || !(mode & ios_base::in))
        return -1;

      streamsize avail = egptr()-gptr(),
        in_avail = static_cast<streamsize>(f.size() - f.tell());
      if(in_avail){
        if((mode & ios_base::binary) || encoding == Encoding::Default)
          in_avail /= sizeof(char_type);
        else if(encoding == Encoding::Utf16)
          in_avail /= sizeof(char16_t);

        // TODO: if we are processing CRLF, this value can be less
        avail += in_avail;
      }
      return avail;
    }

    virtual int_type underflow()
    {
      const int_type eof = traits_type::eof();
      // backup:  gptr-eback [begin,pos) (already readed)
      // pending: egptr-gptr [pos,end)

      streamsize avail = egptr()-gptr();
      if(avail > 0)
        // just return it
        return traits_type::to_int_type(*gptr());
      
      bool ok;
      const bool writeable = (mode&ios_base::out) != 0;
      streamsize cb;
      char_type* p = eback();
      if(!writeable){
        // use all buffer
        if(egptr() == p){
          // possible first call, read 128 characters
          cb = 128;
        }else{
          avail = gptr()-eback();
          avail = avail > 32 ? 16 : (avail <= 1 ? 0 : 1); // leave 16 characters for backup seq if avail
          cb = buf.second - avail;
          if(avail){
            traits_type::move(p, gptr()-1, avail);
            p += avail;
            setg(buf.first, p, p); // temporary set read position to the end of backup
          }
       }
      }else{
        // TODO: implement input/output management
        _assert_msg("not implemented yet");
        return eof;
      }
      if(mode & ios_base::binary){
        ok = read_binary(p, p+cb, cb);
      }else{
        // TODO: process CRLF && ^Z
        EncodingType outenc = encoding;
        if(outenc == Encoding::Default)
          outenc = static_cast<EncodingType>(sizeof(char_type));
        switch(outenc)
        {
        case Encoding::Utf16:
          ok = read_in<wchar_t>(p, p+cb, cb, 
            __::bool_type<__::facets::has_facet<codecvt<char_type,wchar_t,state_type> >::value>());
          break;
        default:
          ok = read_in<char>(p, p+cb, cb,
            __::bool_type<__::facets::has_facet<codecvt<char_type,char,state_type> >::value>());
        }
      }
      if(!ok)
        return eof;

      // [back|pos|readed]
      setg(eback(), p, p+cb);

      return traits_type::to_int_type(*gptr());
    }

    //virtual int_type uflow(); // default

    virtual int_type pbackfail(int_type c = traits::eof())
    {
      const int_type eof = traits_type::eof();
      if(!f)
        return eof;
      _assert_msg("not implemented yet");
      return c;
    }

    virtual streamsize xsputn(const char_type* s, streamsize n)
    {
      if(n <= default_file_buffer_size)
        return basic_streambuf::xsputn(s, n);
      else if(!f)
        return 0;
      // directly put the large data

      // put the pending data
      overflow();
      // request file space & write data
      f.size(f.tell() + n);
      streamsize written;
      write(s, n, &written);
      f.flush();
      return written;
    }

    virtual int_type overflow (int_type c = traits::eof())
    {
      const int_type eof = traits_type::eof();
      const bool eofc = traits_type::eq_int_type(c, eof);
      if(!f || (!pbase() && eofc))
        return eof;

      bool ok = true;
      if(const streamsize pending = pptr()-pbase()) {
        // write buffer
        if(ok = write(pbase(), pending), ok)
          reset();
      }

      if(ok && !eofc){
        // write c
        const char_type cc = traits_type::to_char_type(c);
        ok = write(&cc, 1);
      }
      ok &= flush();
      if(!ok)
        return eof;
      return eofc ? traits_type::not_eof(c) : c;
    }

    virtual basic_streambuf<charT,traits>* setbuf(char_type* s, streamsize n)
    {
      if(!s && !n){
        // unbuffered io
        our_buffer = true;
        reallocate_buffer(1);
        reset();
        return this;
      }
      if(buf.second) reallocate_buffer(0);
      buf.first = s,
        buf.second = n;
      our_buffer = false;
      reset();
      return this;
    }

    virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode = ios_base::in | ios_base::out)
    {
      pos_type re = pos_type(off_type(-1));
      if(!f)
        return re;

      const int width = 
  #if !STLX__CONFORMING_FSTREAM
        // output char size
        ((mode & ios_base::binary) || encoding == Encoding::Default) ? sizeof(char_type) : encoding;
  #else
        use_facet<codecvt<charT,char,typename traits::state_type> >(getloc()).encoding();
  #endif
      if(off != 0 && width <= 0)
        return re;

      if(way != ios_base::cur || off != 0 && pptr()-pbase()){
        if(sync() == -1)
          return re;
      }
      static const native_file::Origin origins[] = {native_file::file_begin, native_file::file_end, native_file::file_current};

      NTL_SUBSYSTEM_NS::file::size_type new_off;
      if(NTL_SUBSYSTEM_NS::success(f.seek(width > 0 ? off*width : 0, origins[way], &new_off))){
        off = new_off;
        if(off != 0 && width > 0)
          off /= width;
        re = pos_type(off);
      }
      return re;
    }

    virtual pos_type seekpos(pos_type sp, ios_base::openmode = ios_base::in | ios_base::out)
    {
      pos_type re = pos_type(off_type(-1));
      if(!f)
        return re;
      _assert_msg("not implemented yet");
      (void)sp;
      return re;
    }

    virtual int sync()
    {
      if(!f)
        return -1;

      const streamsize pending = pptr()-pbase();
      if(pending){
        if(!write(pbase(), pending) || !flush())
          return -1;
        reset();
      }
      return static_cast<int>(pending);
    }

    //virtual void imbue(const locale& loc); // default
    ///\}

    ///////////////////////////////////////////////////////////////////////////
  protected:
    // manage buffer memory
    virtual bool reallocate_buffer(streamsize n)
    {
      if(!our_buffer) // dont manage foreign memory
        return true;

      if(n == 0) {
        // free buffer
        buf.second = n;
        delete[] buf.first;
        return true;

      } else if(n > buf.second) {

        // [re]allocate buffer
        if(buf.second)
          delete[] buf.first;

        buf.first = new char_type[n];
        buf.second = buf.first ? n : 0;
      }
      return buf.second != 0;
    }

    template<typename toT>
    bool write_out(const char_type* from, const char_type* to, streamsize* written, false_type)
    {
      typedef codecvt<toT,char_type,state_type> codecvt;
      static const codecvt& cvt = use_facet<codecvt>(getloc());
      streamsize pending = to-from, actual = 0;
      state_type state;
      const char_type* from_next;
      toT buf[1024], *to_next;
      const void* p;
      do{
        streamsize chunk_size, write_size;
        codecvt_base::result re = cvt.in(state,from,to,from_next,buf,buf+_countof(buf),to_next);
        if(re == codecvt_base::error)
          break;
        else if(re == codecvt_base::noconv)
          p = from, chunk_size = to-from, write_size = chunk_size*sizeof(char_type);
        else // ok | partial
          p = buf, chunk_size = to_next-buf, write_size = chunk_size*sizeof(toT);

        assert(write_size > 0);
        if(!NTL_SUBSYSTEM_NS::success(f.write(p, static_cast<uint32_t>(write_size))))
          break;
        const streamsize fwritten = static_cast<streamsize>(f.get_io_status_block().Information);
        actual += fwritten / (re == codecvt_base::noconv ? sizeof(char_type) : sizeof(toT));
        assert(fwritten == write_size);
        if(fwritten != write_size)
          break;

        pending -= chunk_size;
        from += chunk_size;
      }while(pending > 0);
      if(written) *written = actual;
      return pending == 0;
    }


    template<typename toT>
    bool write_out(const char_type* from, const char_type* to, streamsize* written, true_type)
    {
      typedef codecvt<char_type,toT,state_type> codecvt;
      static const codecvt& cvt = use_facet<codecvt>(getloc());
      streamsize pending = to-from, actual = 0;
      state_type state;
      const char_type* from_next;
      toT buf[1024], *to_next;
      const void* p;
      do{
        streamsize chunk_size, write_size;
        codecvt_base::result re = cvt.out(state,from,to,from_next,buf,buf+_countof(buf),to_next);
        if(re == codecvt_base::error)
          break;
        else if(re == codecvt_base::noconv)
          p = from, chunk_size = to-from, write_size = chunk_size*sizeof(char_type);
        else // ok | partial
          p = buf, chunk_size = to_next-buf, write_size = chunk_size*sizeof(toT);

        assert(write_size > 0);
        if(!NTL_SUBSYSTEM_NS::success(f.write(p, static_cast<uint32_t>(write_size))))
          break;
        const streamsize fwritten = static_cast<streamsize>(f.get_io_status_block().Information);
        actual += fwritten / (re == codecvt_base::noconv ? sizeof(char_type) : sizeof(toT));
        assert(fwritten == write_size);
        if(fwritten != write_size)
          break;

        pending -= chunk_size;
        from += chunk_size;
      }while(pending > 0);
      if(written) *written = actual;
      return pending == 0;
    }
    bool write_binary(const char_type* from, const char_type* to, streamsize* written)
    {
      streamsize pending = to-from, write_size = pending*sizeof(char_type), actual = 0;
      do{
        assert(write_size > 0);
        if(!NTL_SUBSYSTEM_NS::success(f.write(from, static_cast<uint32_t>(write_size))))
          break;
        const streamsize fwritten = static_cast<streamsize>(f.get_io_status_block().Information);
        actual += fwritten;
        assert(fwritten == write_size);
        if(fwritten != pending)
          break;
        pending = 0;
      }while(pending > 0);
      if(written) *written = actual;
      return pending == 0;
    }

    bool read_binary(char_type* to, char_type* to_end, streamsize& readed)
    {
      readed = 0;
      streamsize read_size = to_end - to;
      if(!NTL_SUBSYSTEM_NS::success(f.read(to, static_cast<uint32_t>(read_size))))
        return false;
      readed = static_cast<streamsize>(f.get_io_status_block().Information);
      return readed > 0;
    }

    template<typename fromT>
    bool read_in(char_type* to, char_type* to_end, streamsize& readed, true_type)
    {
      readed = 0;
      streamsize read_size = to_end-to;
      typedef codecvt<char_type,fromT,state_type> codecvt;
      static const codecvt& cvt = use_facet<codecvt>(getloc());
      char_type* to_next;
      fromT buf[1024];
      const fromT *from_next;
      for(;;){
        streamsize cb = min(read_size*sizeof(fromT), _countof(buf));
        if(!NTL_SUBSYSTEM_NS::success(f.read(buf, static_cast<uint32_t>(cb))))
          break;
        streamsize freaded = static_cast<streamsize>(f.get_io_status_block().Information);
        assert(freaded > 0);
        freaded /= sizeof(fromT);

        state_type state;
        codecvt_base::result re = cvt.in(state, buf, buf+freaded, from_next, to, to_end, to_next);
        if(re == codecvt_base::error)
          break;
        else if(re == codecvt_base::noconv){
          readed = freaded;
          traits_type::copy(to, reinterpret_cast<const char_type*>(buf), readed);
        }else{
          readed = to_next-to;
        }
        break;
      }
      return readed > 0;
    }
    template<typename fromT>
    bool read_in(char_type* to, char_type* to_end, streamsize& readed, false_type)
    {
      readed = 0;
      streamsize read_size = to_end-to;
      typedef codecvt<fromT,char_type,state_type> codecvt;
      static const codecvt& cvt = use_facet<codecvt>(getloc());
      char_type* to_next;
      fromT buf[1024];
      const fromT *from_next;
      for(;;){
        streamsize cb = min(read_size*sizeof(fromT), _countof(buf));
        if(!NTL_SUBSYSTEM_NS::success(f.read(buf, static_cast<uint32_t>(cb))))
          break;
        streamsize freaded = static_cast<streamsize>(f.get_io_status_block().Information);
        assert(freaded > 0);      // in bytes
        freaded /= sizeof(fromT); // in chars

        state_type state;
        codecvt_base::result re = cvt.out(state, buf, buf+freaded, from_next, to, to_end, to_next);
        if(re == codecvt_base::error)
          break;
        else if(re == codecvt_base::noconv){
          readed = freaded;
          traits_type::copy(to, reinterpret_cast<const char_type*>(buf), readed);
        }else{
          readed = to_next-to;
        }
        break;
      }
      return readed > 0;
    }

  private:
    bool do_open(const __fs::path& name, ios_base::openmode mode, EncodingType enc)
    {
      if(f)
        return false;

      this->mode = mode;
      const wstring fname = __fs::native(name);

      uint32_t bom = 0, bom_size = 0;
      if(!(mode & (ios_base::binary|ios_base::trunc))){
        // try to read bom when text mode & file isn't truncated
        using namespace NTL_SUBSYSTEM_NS;
        file_handler fh;
        if(success(fh.open(fname, native_file::access_mask_default, native_file::share_valid_flags)) && success(fh.read(&bom,sizeof(bom))))
          bom_size = static_cast<uint32_t>(fh.get_io_status_block().Information);
        fh.close();
      }

      // open file
      native_file::creation_disposition cd;
      native_file::access_mask am;
      native_file::share_mode sm;

      if(mode & ios_base::out){
        // write
        cd = mode & ios_base::trunc ? native_file::overwrite_if : native_file::create_if;
        am = native_file::generic_write;
        sm = native_file::share_read | native_file::share_write; // don't allow delete file when writing
      }else{
        // read
        cd =  native_file::open_existing;
        am = native_file::generic_read;
        sm = native_file::share_valid_flags;
      }
      using namespace NTL_SUBSYSTEM_NS;
      bool ok = success(f.create(fname, cd, am, sm));
      if(!ok)
        return false;

      if(mode & ios_base::ate){
        ok = success(f.seek(0, native_file::file_end));
        if(!ok){
          f.close();
          return false;
        }
      }

      // setup buffer
      if(!buf.second){
        reallocate_buffer(default_file_buffer_size);
        reset();
      }

      // detect encoding on nonempty file
      if(bom_size > 0){
        EncodingType file_enc = parse_encoding(bom, bom_size);
        if(bom_size && (enc == Encoding::Default || file_enc == enc))
          f.seek(bom_size, native_file::file_begin); // skip bom
        if(enc == Encoding::Default)
          enc = file_enc;
      }
      this->encoding = enc;
      return ok;
    }

    bool write(const char_type* from, const char_type* to, streamsize* written = 0)
    {
      return write(from, to-from, written);
    }

    bool write(const char_type* s, streamsize n, streamsize* written = 0)
    {
      if(written) *written = 0;
      if(!n)
        return true;

#if STLX__CONFORMING_FSTREAM
      codecvt_base::result re = codecvt_base::noconv;
      typedef codecvt<charT,char,typename traits::state_type> codecvt;
      const codecvt& a_codecvt = use_facet<codecvt>(getloc());
      char buf[1024], *outpos = buf
        const size_t width = (mode & ios_base::binary) ? sizeof(char_type) : sizeof(char);
#else
      static const size_t width = sizeof(char_type);
      if(!(mode & ios_base::binary) && f.tell() == 0){
        // we are in the unicode text mode, write BOM
        //(UTF-16 LE)
        static const uint32_t bom_le = 0xFEFF;
        uint32_t bom_size = encoding;
        if(encoding == Encoding::Default)
          bom_size = sizeof(char_type); // but utf8 isn't supported by native type
        if(bom_size > 1) // ansi isn't uses bom
          f.write(&bom_le, bom_size);
      }
#endif

      if(mode & ios_base::app)
        f.seek(0, NTL_SUBSYSTEM_NS::file_handler::file_end);

      bool ok = false;
      if(mode & ios_base::binary) {
        ok = write_binary(s, s+n, written);
      }else{
        // TODO: process CRLF && ^Z
        EncodingType outenc = encoding;
        if(outenc == Encoding::Default)
          outenc = static_cast<EncodingType>(sizeof(char_type));
        switch(outenc)
        {
        case Encoding::Utf16:
          ok = write_out<wchar_t>(s, s+n,written, 
            __::bool_type<__::facets::has_facet<codecvt<char_type,wchar_t,state_type> >::value>());
          break;
        default:
          ok = write_out<char>(s, s+n, written,
            __::bool_type<__::facets::has_facet<codecvt<char_type,char,state_type> >::value>());
        }
      }
      return ok;
    }

    bool flush()
    {
      return NTL_SUBSYSTEM_NS::success(f.flush());
    }

    // reset buffer positions
    void reset()
    {
      if(mode&ios_base::in)
        setg(buf.first, buf.first, buf.first); // empty input sequence
      if(mode&ios_base::out)
        setp(buf.first, buf.first+buf.second);
    }

    EncodingType parse_encoding(uint32_t bom, uint32_t& bom_size)
    {
      EncodingType enc = Encoding::Default;
      if(bom_size < 2)
        return bom_size = 0, enc;

      static const uint32_t utf32_be = 0xfffe0000, utf32_le = 0x0000feff,
        utf16_be = 0xfffe, utf16_le = 0xfeff,
        utf8 = 0xBFBBFE;
      
      bom_size = 4;
      for(;;){
        if(bom == utf32_le || bom == utf32_be){ // 4 bytes
          //if(bom == utf32_le) enc = Encoding::Utf32; // not supported
          break;
        }
        bom_size = 3;
        bom &= 0xffffff;
        if(bom == utf8){
          //enc = Encoding::Utf8; 
          break;
        }
        bom_size = 2;
        bom &= 0xffff;
        if(bom == utf16_le || bom == utf32_be){
          if(bom == utf32_le) enc = Encoding::Utf16;
          break;
        }
        bom_size = 0;
        break;
      }
      return enc;
    }

  private:
    //typedef ntl::nt::file_handler native_file;  // for intellisense
    typedef NTL_SUBSYSTEM_NS::file_handler native_file;

    native_file f;
    pair<char_type*, streamsize> buf;
    EncodingType encoding;
    ios_base::openmode mode;
    bool our_buffer;
  };


/**
 *	@brief 27.9.1.6 Class template basic_ifstream [ifstream]
 *  @details The class basic_ifstream<charT, traits> supports reading from named files.
 *  It uses a basic_filebuf<charT, traits> object to control the associated sequence.
 **/
template <class charT, class traits /*= char_traits<charT>*/ >
class basic_ifstream:
  public basic_istream<charT, traits>
{
  public:
    typedef charT                     char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits                    traits_type;

    ///\name 27.9.1.7 basic_ifstream constructors [ifstream.cons]

    basic_ifstream()
      :basic_istream(&sb)
    {}

    explicit basic_ifstream(const char* s, ios_base::openmode mode = ios_base::in, EncodingType encoding = Encoding::Ansi)
      :basic_istream(&sb)
    {
      open(s, mode, encoding);
    }
    explicit basic_ifstream(const string& s, ios_base::openmode mode = ios_base::in, EncodingType encoding = Encoding::Ansi)
      :basic_istream(&sb)
    {
      open(s, mode, encoding);
    }
#ifdef NTL_CXX_RV
    basic_ifstream(basic_ifstream&& rhs)
      : basic_istream(std::move(rhs))
      , sb(std::move(rhs.sb))
    {
      this->set_rdbuf(&sb);
    }
#endif

    // NTL extension
    explicit basic_ifstream(const __fs::path& name, ios_base::openmode mode = ios_base::in, EncodingType encoding = Encoding::Ansi)
      :basic_istream(&sb)
    {
      open(name, mode, encoding);
    }
    ///\name 27.9.1.8 Assign and swap [ifstream.assign]
#ifdef NTL_CXX_RV
    basic_ifstream& operator=(basic_ifstream&& rhs)
    {
      basic_istream::operator =(std::move(rhs));
      sb = std::move(rhs.sb);
      return *this;
    }
#endif
    void swap(basic_ifstream& rhs)
    {
      basic_istream::swap(rhs);
      sb.swap(rhs.sb);
    }

    ///\name 27.9.1.9 Member functions [ifstream.members]
    basic_filebuf<charT,traits>* rdbuf() const { return const_cast<basic_filebuf<charT,traits>*>(&sb); }

    bool is_open() const { return sb.is_open(); }

    void open(const char* s, ios_base::openmode mode = ios_base::in, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(s, mode|ios_base::in, encoding))
        setstate(ios_base::failbit);
    }
    void open(const string& s, ios_base::openmode mode = ios_base::in, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(s, mode|ios_base::in, encoding))
        setstate(ios_base::failbit);
    }
    // NTL extension
    void open(const __fs::path& name, ios_base::openmode mode = ios_base::in, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(name, mode|ios_base::in, encoding))
        setstate(ios_base::failbit);
    }
    void close()
    {
      if(!sb.close())
        setstate(ios_base::failbit);
    }

  private:
    basic_filebuf<charT,traits> sb;

    basic_ifstream(const basic_ifstream&) __deleted;
    basic_ifstream& operator=(const basic_ifstream&) __deleted;
};


/**
 *	@brief 27.9.1.10 Class template basic_ofstream [ofstream]
 *  @details The class basic_ofstream<charT, traits> supports writing to named files.
 *  It uses a basic_filebuf<charT, traits> object to control the associated sequence.
 **/
template <class charT, class traits /*= char_traits<charT>*/ >
class basic_ofstream:
  public basic_ostream<charT, traits>
{
  public:
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits traits_type;

    ///\name 27.9.1.11 basic_ofstream constructors [ofstream.cons]
    basic_ofstream()
      :basic_ostream(&sb)
    {}
    explicit basic_ofstream(const char* s, ios_base::openmode mode = ios_base::out, EncodingType encoding = Encoding::Ansi)
      :basic_ostream(&sb)
    {
      open(s, mode, encoding);
    }
    explicit basic_ofstream(const string& s, ios_base::openmode mode = ios_base::out, EncodingType encoding = Encoding::Ansi)
      :basic_ostream(&sb)
    {
      open(s, mode, encoding);
    }
#ifdef NTL_CXX_RV
    basic_ofstream(basic_ofstream&& rhs)
      : basic_ostream<charT,traits>(std::move(rhs))
      , sb(std::move(rhs.sb))
    {
      this->set_rdbuf(&sb);
    }

#endif

    // NTL extension
    explicit basic_ofstream(const __fs::path& name, ios_base::openmode mode = ios_base::out, EncodingType encoding = Encoding::Ansi)
      :basic_ostream(&sb)
    {
      open(name, mode, encoding);
    }
    ///\name 27.9.1.12 Assign and swap [ofstream.assign]
#ifdef NTL_CXX_RV
    basic_ofstream& operator=(basic_ofstream&& rhs)
    {
      basic_ostream::operator =(std::move(rhs));
      sb = std::move(rhs.sb);
      return *this;
    }
#endif

    void swap(basic_ofstream& rhs)
    {
      basic_ostream::swap(rhs);
      sb.swap(rhs.sb);
    }

    ///\name 27.9.1.13 Member functions [ofstream.members]
    basic_filebuf<charT,traits>* rdbuf() const { return const_cast<basic_filebuf<charT,traits>*>(&sb); }
    
    bool is_open() const { return sb.is_open(); }

    void open(const char* s, ios_base::openmode mode = ios_base::out, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(s, mode|ios_base::out, encoding))
        setstate(ios_base::failbit);
    }
    void open(const string& s, ios_base::openmode mode = ios_base::out, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(s, mode|ios_base::out, encoding))
        setstate(ios_base::failbit);
    }
    // NTL extension
    void open(const __fs::path& name, ios_base::openmode mode = ios_base::out, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(name, mode|ios_base::out, encoding))
        setstate(ios_base::failbit);
    }

    void close()
    {
      if(!sb.close())
        setstate(ios_base::failbit);
    }

  ///////////////////////////////////////////////////////////////////////////
  private:
    basic_filebuf<charT,traits> sb;

    basic_ofstream(const basic_ofstream&) __deleted;
    basic_ofstream& operator=(const basic_ofstream&) __deleted;
};


/**
 *	@brief 27.9.1.14 Class template basic_fstream [fstream]
 *  @details The class template basic_fstream<charT,traits> supports reading and writing from named files.
 *  It uses a basic_filebuf<charT,traits> object to control the associated sequences. 
 **/
template <class charT, class traits /*= char_traits<charT>*/ >
class basic_fstream:
  public basic_iostream<charT, traits>
{
  public:
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits traits_type;

    ///\name 27.9.1.15 basic_fstream constructors [fstream.cons]
    basic_fstream()
      :basic_iostream(&sb)
    {}
    explicit basic_fstream(const char* s, ios_base::openmode mode = ios_base::in|ios_base::out, EncodingType encoding = Encoding::Ansi)
      :basic_iostream(&sb)
    {
      open(s, mode, encoding);
    }
    explicit basic_fstream(const string& s, ios_base::openmode mode = ios_base::in|ios_base::out, EncodingType encoding = Encoding::Ansi)
      :basic_iostream(&sb)
    {
      open(s, mode, encoding);
    }
#ifdef NTL_CXX_RV
    basic_fstream(basic_fstream&& rhs)
      : basic_iostream<charT,traits>(std::move(rhs))
      , sb(std::move(rhs.sb))
    {
      this->set_rdbuf(&sb);
    }

#endif

    // NTL extension
    explicit basic_fstream(const __fs::path& name, ios_base::openmode mode = ios_base::in|ios_base::out, EncodingType encoding = Encoding::Ansi)
      :basic_iostream(&sb)
    {
      open(name, mode, encoding);
    }
    ///\name 27.9.1.16 Assign and swap [fstream.assign]
#ifdef NTL_CXX_RV
    basic_fstream& operator=(basic_fstream&& rhs)
    {
      basic_iostream::operator =(std::move(rhs));
      sb = std::move(rhs.sb);
      return *this;
    }
#endif

    void swap(basic_fstream& rhs)
    {
      basic_iostream::swap(rhs);
      sb.swap(rhs.sb);
    }

    ///\name  27.9.1.17 Member functions [fstream.members]
    basic_filebuf<charT,traits>* rdbuf() const { return const_cast<basic_filebuf<charT,traits>*>(&sb); }

    bool is_open() const { return sb.is_open(); }

    void open(const char* s, ios_base::openmode mode = ios_base::in|ios_base::out, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(s, mode, encoding))
        setstate(ios_base::failbit);
    }
    void open(const string& s, ios_base::openmode mode = ios_base::in|ios_base::out, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(s, mode, encoding))
        setstate(ios_base::failbit);
    }
    // NTL extension
    void open(const __fs::path& name, ios_base::openmode mode = ios_base::in|ios_base::out, EncodingType encoding = Encoding::Ansi)
    {
      if(!sb.open(name, mode, encoding))
        setstate(ios_base::failbit);
    }
    void close()
    {
      if(!sb.close())
        setstate(ios_base::failbit);
    }
  private:
    basic_filebuf<charT,traits> sb;

    basic_fstream(const basic_fstream&) __deleted;
    basic_fstream& operator=(const basic_fstream&) __deleted;
};

//////////////////////////////////////////////////////////////////////////

template <class charT, class traits>
inline void swap(basic_filebuf<charT, traits>& x, basic_filebuf<charT, traits>& y)   { x.swap(y); }

template <class charT, class traits>
inline void swap(basic_ifstream<charT, traits>& x, basic_ifstream<charT, traits>& y) { x.swap(y); }

template <class charT, class traits>
inline void swap(basic_ofstream<charT, traits>& x, basic_ofstream<charT, traits>& y) { x.swap(y); }

//////////////////////////////////////////////////////////////////////////
/**@} lib_file_streams */
/**@} lib_string_streams */
}//namespace std
#endif//#ifndef NTL__STLX_FSTREAM
