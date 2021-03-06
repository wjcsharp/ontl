/**\file*********************************************************************
 *                                                                     \brief
 *  Buffers
 *
 ****************************************************************************
 */
#ifndef NTL__STLX_TR2_BUFFER
#define NTL__STLX_TR2_BUFFER
#pragma once

#include "../../../utility.hxx"
#include "../../../array.hxx"
#include "../../../stdstring.hxx"
#include "../../../streambuf.hxx"
#include "../../../system_error.hxx"

#include "io_service_fwd.hxx"

namespace std { namespace tr2 { namespace sys {

  class const_buffer;
  class mutable_buffer;

  size_t buffer_size(const const_buffer& b) __ntl_nothrow;
  size_t buffer_size(const mutable_buffer& b) __ntl_nothrow;


  /**
   *  @brief 5.5.3. Class mutable_buffer
   **/
  class mutable_buffer:
    protected pair<void*, size_t>
  {
    friend class const_buffer;
  public:
    mutable_buffer()
    {}

    mutable_buffer(void* data, size_t size)
      :pair(data, size)
    {}

    mutable_buffer(const mutable_buffer& b)
      :pair(b)
    {}

    template<class T> 
    friend inline T buffer_cast(const mutable_buffer& b) __ntl_nothrow;
    friend size_t buffer_size(const mutable_buffer& b);
  };



  /**
   *  @brief 5.5.4. Class const_buffer
   **/
  class const_buffer:
    protected pair<const void*, size_t>
  {
  public:
    const_buffer()
    {}
    const_buffer(const void* data, size_t size)
      :pair(data, size)
    {}
    const_buffer(const mutable_buffer& b)
      :pair(b)
    {}

    template<class T>
    friend inline T buffer_cast(const const_buffer& b) __ntl_nothrow;
    friend size_t buffer_size(const const_buffer& b) __ntl_nothrow;
  };

  template<class T>
  inline T buffer_cast(const const_buffer& b) __ntl_nothrow        { return static_cast<T>(b.first); }
  template<class T> 
  inline T buffer_cast(const mutable_buffer& b) __ntl_nothrow      { return static_cast<T>(b.first); }

  inline size_t buffer_size(const const_buffer& b) __ntl_nothrow    { return b.second; }
  inline size_t buffer_size(const mutable_buffer& b) __ntl_nothrow  { return b.second; }



  /**
   *  @brief 5.5.5. Class mutable_buffers_1
   **/
  class mutable_buffers_1:
    public mutable_buffer
  {
  public:
    // types:
    typedef mutable_buffer  value_type;
    typedef const mutable_buffer* const_iterator;

    // constructors:
    mutable_buffers_1(void* data, size_t size)
      :mutable_buffer(data, size)
    {}

    explicit mutable_buffers_1(const mutable_buffer& b)
      :mutable_buffer(b)
    {}

    // members:
    const_iterator begin() const { return static_cast<const_iterator>(this);   }
    const_iterator end()   const { return static_cast<const_iterator>(this)+1; }
  };



  /**
   *  @brief 5.5.6. Class const_buffers_1
   **/
  class const_buffers_1:
    public const_buffer
  {
  public:
    // types:
    typedef const_buffer  value_type;
    typedef const const_buffer* const_iterator;

    // constructors:
    const_buffers_1(const void* data, size_t size)
      :const_buffer(data, size)
    {}
    explicit const_buffers_1(const const_buffer& b)
      :const_buffer(b)
    {}

    // members:
    const_iterator begin() const { return static_cast<const_iterator>(this);   }
    const_iterator end()   const { return static_cast<const_iterator>(this)+1; }
  };


  /**
   *  @brief 5.5.8. Class template basic_fifobuf
   *
   *  The class basic_fifobuf is derived from basic_streambuf to associate the input sequence and output sequence with one 
   *  or more objects of some character array type, whose elements store arbitrary values. These character array objects are internal 
   *  to the basic_fifobuf object, but direct access to the array elements is provided to permit them to be used with I/O 
   *  operations, such as the \c send or \c receive operations of a socket. Characters written to the output sequence of a 
   *  basic_fifobuf object are appended to the input sequence of the same object.
   *
   *  The class basic_fifobuf permits the following implementation strategies: 
   *  - A single contiguous character array, which is reallocated as necessary to accommodate changes in the size of the character sequence. 
   *  - A sequence of one or more character arrays, where each array is of the same size. Additional character array objects 
   *    are appended to the sequence to accommodate changes in the size of the character sequence. 
   *  - A sequence of one or more character arrays of varying sizes. Additional character array objects are appended to the 
   *    sequence to accommodate changes in the size of the character sequence.
   **/
  template<class Allocator = std::allocator<char> >
  class basic_fifobuf
    : public std::streambuf
    , ntl::noncopyable
  {
  public:
    // types:
    typedef Allocator         allocator_type;
    typedef const_buffers_1   const_buffers_type;
    typedef mutable_buffers_1 mutable_buffers_type;

  public:
    // constructors:
    explicit basic_fifobuf(size_t max_sz = numeric_limits<size_t>::max(), const Allocator& alloc = Allocator())
      : buffer_(alloc)
      , max_size_(max_sz)
    {
      size_t capacity = std::min(max_sz, initial_output_size);
      buffer_.resize(std::max<size_t>(capacity, 1u));  // at least 1 byte
      setg(&buffer_[0], &buffer_[0], &buffer_[0]);
      setp(&buffer_[0], &buffer_[0] + capacity);
    }

    // members:
    allocator_type get_allocator() const { return buffer_.get_allocator(); }

    size_t size()     const { return pptr() - gptr(); }
    size_t max_size() const { return max_size_;       }

    const_buffers_type data() const { return buffer(const_buffers_type(gptr(), size())); }

    /** Ensures that the output sequence can accommodate n characters, reallocating character array objects as necessary.  */
    mutable_buffers_type prepare(size_t n)
    {
      growto(n);
      return buffer(pptr(), n);
    }
    
    /** Appends \p n characters from the start of the output sequence to the input sequence. The beginning of the output sequence is advanced by \p n characters. */
    void commit(size_t n) __ntl_throws(length_error)
    {
      if(pptr() + n > epptr())
        n = epptr() - pptr();
      
      pbump(static_cast<int>(n));
      setg(eback(), gptr(), pptr());
    }

    /** Removes \p n characters from the beginning of the input sequence.  */
    void consume(size_t n) __ntl_throws(length_error)
    {
      if(egptr() < pptr())
        setg(&buffer_[0], gptr(), pptr());

      if(gptr() + n > pptr())
        n = pptr() - gptr();
      gbump(static_cast<int>(n));
    }

  protected:
    // overridden virtual functions:
    virtual int_type underflow() override
    {
      if(gptr() < pptr()) {
        setg(&buffer_[0], gptr(), pptr());
        return traits_type::to_int_type(*gptr());
      }
      return traits_type::eof();
    }

    //virtual int_type pbackfail(int_type c = traits_type::eof());
    virtual int_type overflow(int_type c = traits_type::eof()) override
    {
      if(traits_type::eq_int_type(c, traits_type::eof()))
        return traits_type::not_eof(c);

      if(pptr() == epptr()) {
        size_t cb = pptr() - gptr();
        if(cb < max_size_ && (max_size_ - cb) < initial_output_size)
          growto(max_size_ - cb);
        else
          growto(initial_output_size);
      }
      *pptr() = traits_type::to_char_type(c);
      pbump(1);
      return c;
    }
  
  protected:
    void growto(size_t n)
    {
      size_t gnext = gptr() - &buffer_[0];
      size_t pnext = pptr() - &buffer_[0];
      size_t pend = epptr() - &buffer_[0];

      // Check if there is already enough space in the put area.
      if (n <= pend - pnext)
      {
        return;
      }

      // Shift existing contents of get area to start of buffer.
      if (gnext > 0)
      {
        pnext -= gnext;
        std::memmove(&buffer_[0], &buffer_[0] + gnext, pnext);
      }

      // Ensure buffer is large enough to hold at least the specified size.
      if (n > pend - pnext)
      {
        if (n <= max_size_ && pnext <= max_size_ - n)
        {
          pend = pnext + n;
          buffer_.resize((std::max<std::size_t>)(pend, 1));
        }
        else
        {
          __throw_length_error(__func__": `n` too large");
        }
      }

      setg(&buffer_[0], &buffer_[0], &buffer_[0] + pnext);
      setp(&buffer_[0] + pnext, &buffer_[0] + pend);
    }


  private:
    basic_fifobuf(const basic_fifobuf&) __deleted;
    void operator=(const basic_fifobuf&)__deleted;

  private:
    static const size_t initial_output_size = 64;

    std::string buffer_;
    size_t max_size_;
  };


  /** Typedef for the typical usage of basic_fifobuf. */
  typedef basic_fifobuf<> fifobuf, streambuf;


  /**
   *	@brief 5.5.9. Class transfer_all
   *	
   *  A completion condition function object that indicates that a read or write operation should continue until all of the data has been transferred,
   *  or until an error occurs.
   **/
  class transfer_all:
    public binary_function<error_code, size_t, bool>
  {
  public:
    static const size_t max_transfer = 1024*64;

    size_t operator()(const error_code& ec, size_t) const
    {
      return !!ec ? 0 : max_transfer;
    }
  };


  /**
   *	@brief 5.5.10. Class transfer_at_least
   *	
   *	A completion condition function object that indicates that a read or write operation
   *	should continue until a minimum number of bytes has been transferred,
   *	or until an error occurs.
   **/
  class transfer_at_least:
    public binary_function<error_code, size_t, bool>
  {
  public:
    transfer_at_least(size_t m)
      :min(m)
    {}
    size_t operator()(const error_code& ec, size_t s) const
    {
      return (!!ec || s >= min) ? 0 : transfer_all::max_transfer;
    }
  private:
    size_t min;
  };


  /**
   *	@brief Class transfer_exactly
   *	
   *	A completion condition function object that indicates that a read or write operation
   *	should continue until an exact number of bytes has been transferred,
   *	or until an error occurs.
   **/
  class transfer_exactly:
    public binary_function<error_code, size_t, bool>
  {
  public:
    transfer_exactly(size_t m)
      :min(m)
    {}
    size_t operator()(const error_code& ec, size_t s) const
    {
      return (!!ec || s >= min) ? 0 : std::min(min - s, transfer_all::max_transfer);
    }
  private:
    size_t min;
  };


  
  inline const_buffer operator+(const const_buffer& b, size_t size) __ntl_nothrow
  {
    return const_buffer(buffer_cast<const char*>(b) + min(size, buffer_size(b)),
      buffer_size(b) - min(size, buffer_size(b)));
  }
  inline const_buffer operator+(size_t size, const const_buffer& b) __ntl_nothrow
  {
    return b + size;
  }

  inline mutable_buffer operator+(const mutable_buffer& b, size_t size) __ntl_nothrow
  {
    return mutable_buffer(buffer_cast<char*>(b) + min(size, buffer_size(b)),
      buffer_size(b) - min(size, buffer_size(b)));
  }
  inline mutable_buffer operator+(size_t size, const mutable_buffer& b) __ntl_nothrow
  {
    return b + size;
  }




  ///\name 5.5.7. Buffer creation functions
  inline mutable_buffers_1  buffer(void* p, size_t s) __ntl_nothrow { return mutable_buffers_1(p,s); }
  inline const_buffers_1    buffer(const void* p, size_t s) __ntl_nothrow { return const_buffers_1(p,s); }
  inline mutable_buffers_1  buffer(const mutable_buffer& b) __ntl_nothrow { return mutable_buffers_1(b); }
  inline mutable_buffers_1  buffer(const mutable_buffer& b, size_t s) __ntl_nothrow { return mutable_buffers_1(buffer_cast<void*>(b), min(buffer_size(b), s)); }
  inline const_buffers_1    buffer(const const_buffer& b) __ntl_nothrow { return const_buffers_1(b); }
  inline const_buffers_1    buffer(const const_buffer& b, size_t s) __ntl_nothrow { return const_buffers_1(buffer_cast<const void*>(b),min(buffer_size(b), s)); }

  template<class T, size_t N>
  inline mutable_buffers_1 buffer(T (&arr)[N]) { return mutable_buffers_1(static_cast<void*>(arr), N * sizeof(T)); }
  template<class T, size_t N>
  inline mutable_buffers_1 buffer(T (&arr)[N], size_t s) { return mutable_buffers_1(static_cast<void*>(arr), min(N * sizeof(T), s)); }
  template<class T, size_t N>
  inline const_buffers_1 buffer(const T (&arr)[N]) { return const_buffers_1(static_cast<const void*>(arr), N * sizeof(T)); }
  template<class T, size_t N>
  inline const_buffers_1 buffer(const T (&arr)[N], size_t s) { return const_buffers_1(static_cast<const void*>(arr), min(N * sizeof(T), s)); }
  
  template<class T, size_t N>
  inline mutable_buffers_1 buffer(array<T, N>& arr) { return mutable_buffers_1(arr.data(), arr.size() * sizeof(T)); }
  template<class T, size_t N>
  inline mutable_buffers_1 buffer(array<T, N>& arr, size_t s) { return mutable_buffers_1(arr.data(), min(arr.size() * sizeof(T), s)); }
  template<class T, size_t N>
  inline const_buffers_1 buffer(array<const T, N>& arr) { return const_buffers_1( arr.data(), arr.size() * sizeof(T)); }
  template<class T, size_t N>
  inline const_buffers_1 buffer(array<const T, N>& arr, size_t s) { return const_buffers_1( arr.data(), min(arr.size() * sizeof(T), s)); }
  template<class T, size_t N>
  inline const_buffers_1 buffer(const array<T, N>& arr) { return const_buffers_1( arr.data(), arr.size() * sizeof(T)); }
  template<class T, size_t N>
  inline const_buffers_1 buffer(const array<T, N>& arr, size_t s) { return const_buffers_1( arr.data(), min(arr.size() * sizeof(T), s)); }

  template<class T, class Allocator>
  inline mutable_buffers_1 buffer(vector<T, Allocator>& vec) { return mutable_buffers_1( vec.size() ? &vec[0] : 0, vec.size() * sizeof(T)); }
  template<class T, class Allocator>
  inline mutable_buffers_1 buffer(vector<T, Allocator>& vec, size_t s) { return mutable_buffers_1( vec.size() ? &vec[0] : 0, min(vec.size() * sizeof(T), s)); }
  template<class T, class Allocator>
  inline const_buffers_1 buffer(const vector<T, Allocator>& vec) { return const_buffers_1( vec.size() ? &vec[0] : 0, vec.size() * sizeof(T)); }
  template<class T, class Allocator>
  inline const_buffers_1 buffer(const vector<T, Allocator>& vec, size_t s) { return const_buffers_1( vec.size() ? &vec[0] : 0, min(vec.size() * sizeof(T), s)); }

  template<class CharT, class Traits, class Allocator>
  inline mutable_buffers_1 buffer(basic_string<CharT, Traits, Allocator>& str) { return mutable_buffers_1(str.begin(), str.size() * sizeof(CharT)); }
  template<class CharT, class Traits, class Allocator>
  inline mutable_buffers_1 buffer(basic_string<CharT, Traits, Allocator>& str, size_t s) { return mutable_buffers_1(str.begin(), min(str.size() * sizeof(CharT), s)); }
  template<class CharT, class Traits, class Allocator>
  inline const_buffers_1 buffer(const basic_string<CharT, Traits, Allocator>& str) { return const_buffers_1(str.cbegin(), str.size() * sizeof(CharT)); }
  template<class CharT, class Traits, class Allocator>
  inline const_buffers_1 buffer(const basic_string<CharT, Traits, Allocator>& str, size_t s) { return const_buffers_1(str.cbegin(), min(str.size() * sizeof(CharT), s)); }

  template<class CharT, class Traits>
  inline mutable_buffers_1 buffer(basic_string_ref<CharT, Traits>& str) { return mutable_buffers_1(str.begin(), str.size() * sizeof(CharT)); }
  template<class CharT, class Traits>
  inline mutable_buffers_1 buffer(basic_string_ref<CharT, Traits>& str, size_t s) { return mutable_buffers_1(str.begin(), min(str.size() * sizeof(CharT), s)); }
  template<class CharT, class Traits>
  inline const_buffers_1 buffer(const basic_string_ref<CharT, Traits>& str) { return const_buffers_1(str.cbegin(), str.size() * sizeof(CharT)); }
  template<class CharT, class Traits>
  inline const_buffers_1 buffer(const basic_string_ref<CharT, Traits>& str, size_t s) { return const_buffers_1(str.cbegin(), min(str.size() * sizeof(CharT), s)); }



  ///\name 5.5.11. Synchronous read operations

  /** This function is used to read a whole buffer from a stream. */
  template<class SyncReadStream, class MutableBufferSequence>
  inline size_t read(SyncReadStream& stream, const MutableBufferSequence& buffers)
  {
    error_code ec;
    size_t s = read(stream, buffers, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }
  
  /** This function is used to read a whole buffer from a stream. */
  template<class SyncReadStream, class MutableBufferSequence>
  inline size_t read(SyncReadStream& stream, const MutableBufferSequence& buffers, error_code& ec)
  {
    return read(stream, buffers, transfer_all(), ec);
  }

  /** This function is used to read a whole buffer from a stream until condition returns \c 0. */
  template<class SyncReadStream, class MutableBufferSequence, class CompletionCondition>
  inline size_t read(SyncReadStream& stream, const MutableBufferSequence& buffers, CompletionCondition condition)
  {
    error_code ec;
    size_t s = read(stream, buffers, condition, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }

  /** This function is used to read a whole buffer from a stream until condition returns \c 0. */
  template<class SyncReadStream, class MutableBufferSequence, class CompletionCondition>
  inline size_t read(SyncReadStream& stream, const MutableBufferSequence& buffers, CompletionCondition condition, error_code& ec)
  {
    ec.clear();
    const size_t max_size = buffer_size(buffers);
    uint8_t* buf = buffer_cast<uint8_t*>(buffers);
    size_t transferred = 0;
    while(transferred < max_size) {
      const size_t max_transfer = std::min(condition(ec, transferred), max_size - transferred);
      if(max_transfer == 0)
        break;
      const size_t re = stream.read_some(buffer(buf + transferred, max_transfer), ec);
      transferred += re;
    }
    return transferred;
  }

  template<class SyncReadStream, class Allocator>
  inline size_t read(SyncReadStream& stream, basic_fifobuf<Allocator>& fb)
  {
    error_code ec;
    size_t s = read(stream, fb, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }

  template<class SyncReadStream, class Allocator>
  size_t read(SyncReadStream& stream, basic_fifobuf<Allocator>& fb, error_code& ec);

  template<class SyncReadStream, class Allocator, class CompletionCondition>
  inline size_t read(SyncReadStream& stream, basic_fifobuf<Allocator>& fb, CompletionCondition condition)
  {
    error_code ec;
    size_t s = read(stream, fb, condition, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }

  template<class SyncReadStream, class Allocator, class CompletionCondition>
  size_t read(SyncReadStream& stream, basic_fifobuf<Allocator>& fb, CompletionCondition condition, error_code& ec);


  /** This function is used to receive a whole buffer from a stream until condition returns \c 0. */
  template<class SyncReadStream, class MutableBufferSequence, class CompletionCondition>
  inline size_t receive(SyncReadStream& stream, const MutableBufferSequence& buffers, CompletionCondition condition, error_code& ec)
  {
    return read(stream, buffers, condition, ec);
  }

  /** This function is used to receive a whole buffer from a stream. */
  template<class SyncReadStream, class MutableBufferSequence>
  inline size_t receive(SyncReadStream& stream, const MutableBufferSequence& buffers, error_code& ec)
  {
    return read(stream, buffers, ec);
  }

  ///\name 5.5.12. Asynchronous write operations
  template<class AsyncReadStream, class Allocator, class ReadHandler>
  size_t async_read(AsyncReadStream& stream, basic_fifobuf<Allocator>& fb, ReadHandler handler);

  template<class AsyncReadStream, class Allocator, class CompletionCondition, class ReadHandler>
  size_t async_read(AsyncReadStream& stream, basic_fifobuf<Allocator>& fb, CompletionCondition condition, ReadHandler handler);




  ///\name 5.5.13. Synchronous write operations

  /** This function is used to write a whole buffer to the stream. */
  template<class SyncWriteStream, class ConstBufferSequence>
  inline size_t write(SyncWriteStream& stream, const ConstBufferSequence& buffers)
  {
    error_code ec;
    size_t s = write(stream, buffers, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }
  
  /** This function is used to write a whole buffer to the stream. */
  template<class SyncWriteStream, class ConstBufferSequence>
  inline size_t write(SyncWriteStream& stream, const ConstBufferSequence& buffers, error_code& ec)
  {
    return write(stream, buffers, transfer_all(), ec);
  }

  /** This function is used to write a whole buffer to the stream until \c condition returns 0. */
  template<class SyncWriteStream, class ConstBufferSequence, class CompletionCondition>
  inline size_t write(SyncWriteStream& stream, const ConstBufferSequence& buffers, CompletionCondition condition)
  {
    error_code ec;
    size_t s = write(stream, buffers, condition, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }

  /** This function is used to write a whole buffer to the stream until \c condition returns 0. */
  template<class SyncWriteStream, class ConstBufferSequence, class CompletionCondition>
  inline size_t write(SyncWriteStream& stream, const ConstBufferSequence& buffers, CompletionCondition condition, error_code& ec)
  {
    ec.clear();
    const size_t max_size = buffer_size(buffers);
    const uint8_t* buf = buffer_cast<const uint8_t*>(buffers);
    size_t transferred = 0;
    while(transferred < max_size) {
      const size_t max_transfer = std::min(condition(ec, transferred), max_size - transferred);
      if(max_transfer == 0)
        break;
      const size_t re = stream.write_some(buffer(buf + transferred, max_transfer), ec);
      transferred += re;
    }
    return transferred;
  }

  template<class SyncWriteStream, class Allocator>
  inline size_t write(SyncWriteStream& stream, basic_fifobuf<Allocator>& fb)
  {
    error_code ec;
    size_t s = write(stream, fb, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }

  template<class SyncWriteStream, class Allocator>
  size_t write(SyncWriteStream& stream, basic_fifobuf<Allocator>& fb, error_code& ec);

  template<class SyncWriteStream, class Allocator, class CompletionCondition>
  inline size_t write(SyncWriteStream& stream, basic_fifobuf<Allocator>& fb, CompletionCondition condition)
  {
    error_code ec;
    size_t s = write(stream, fb, condition, ec);
    if(ec)
      __ntl_throw(system_error(ec));
    return s;
  }

  template<class SyncWriteStream, class Allocator, class CompletionCondition>
  size_t write(SyncWriteStream& stream, basic_fifobuf<Allocator>& fb, CompletionCondition condition, error_code& ec);


  /** This function is used to send a whole buffer to the stream until \c condition returns 0. */
  template<class SyncWriteStream, class ConstBufferSequence, class CompletionCondition>
  inline size_t send(SyncWriteStream& stream, const ConstBufferSequence& buffers, CompletionCondition condition, error_code& ec)
  {
    return write(stream, buffers, condition, ec);
  }

  /** This function is used to send a whole buffer to the stream. */
  template<class SyncWriteStream, class ConstBufferSequence>
  inline size_t send(SyncWriteStream& stream, const ConstBufferSequence& buffers, error_code& ec)
  {
    return write(stream, buffers, ec);
  }


  ///\name 5.5.14. Asynchronous write operations
  template<class AsyncWriteStream, class ConstBufferSequence, class WriteHandler>
  size_t async_write(AsyncWriteStream& stream, const ConstBufferSequence& buffers, WriteHandler handler);
  template<class AsyncWriteStream, class ConstBufferSequence, class CompletionCondition, class WriteHandler>
  size_t async_write(AsyncWriteStream& stream, const ConstBufferSequence& buffers, CompletionCondition condition, WriteHandler handler);

  template<class AsyncWriteStream, class Allocator, class WriteHandler>
  size_t async_write(AsyncWriteStream& stream, basic_fifobuf<Allocator>& fb, WriteHandler handler);
  template<class AsyncWriteStream, class Allocator, class CompletionCondition, class WriteHandler>
  size_t async_write(AsyncWriteStream& stream, basic_fifobuf<Allocator>& fb, CompletionCondition condition, WriteHandler handler);




  ///\name 5.5.15. Synchronous delimited read operations
    template <class SyncReadStream, class Allocator>
  inline size_t read_until(SyncReadStream& s, basic_fifobuf<Allocator>& fb, char delim)
  {
    error_code ec;
    size_t cb = read_until(s, fb, delim);
    if(ec)
      __ntl_throw(system_error(ec));
    return cb;
  }
  template <class SyncReadStream, class Allocator>
  size_t read_until(SyncReadStream& s, basic_fifobuf<Allocator>& fb, char delim, error_code& ec);
  template <class SyncReadStream, class Allocator>
  inline size_t read_until(SyncReadStream& s, basic_fifobuf<Allocator>& fb, const string& delim)
  {
    error_code ec;
    size_t cb = read_until(s, fb, delim);
    if(ec)
      __ntl_throw(system_error(ec));
    return cb;
  }
  template <class SyncReadStream, class Allocator>
  size_t read_until(SyncReadStream& s, basic_fifobuf<Allocator>& fb, const string& delim, error_code& ec);




  ///\name 5.5.16. Asynchronous delimited read operations
  template <class AsyncReadStream, class Allocator, class ReadHandler>
  void async_read_until(AsyncReadStream& s, basic_fifobuf<Allocator>& fb, char delim, ReadHandler handler);
  template <class AsyncReadStream, class Allocator, class ReadHandler>
  void async_read_until(AsyncReadStream& s, basic_fifobuf<Allocator>& fb, const string& delim, ReadHandler handler);

  ///\}

}}}
#endif // NTL__STLX_TR2_BUFFER
