#ifndef Podd_DataBuffer_h
#define Podd_DataBuffer_h

//////////////////////////////////////////////////////////////////////////
//
// Podd::DataBuffer
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <cassert>

namespace Podd {

  class DataBuffer {
  public:
    explicit DataBuffer( size_t n = 0 );
    DataBuffer( const DataBuffer& rhs );
    DataBuffer& operator=( const DataBuffer& rhs);
#ifndef __CINT__
#if __cplusplus >= 201103L
    DataBuffer( DataBuffer&& rhs ) noexcept;
    DataBuffer& operator=( DataBuffer&& rhs) noexcept;
#endif
#endif
    virtual ~DataBuffer();

    void   Clear();
    Int_t  Compact( size_t n = 0 );
    Int_t  Fill( const void* ptr, size_t nbytes, size_t i = 0 );
    char*  Get() const { return fData; }
    size_t GetAllocation() const { return fNalloc; }
    size_t GetSize() const { return fNdata; }
    Int_t  Reserve( size_t n );
    void   SetSize( size_t n );

  private:
    char*   fData;    // Data buffer
    size_t  fNalloc;  // Bytes allocated
    size_t  fNdata;   // Bytes used

    Int_t Realloc( size_t newsize );

    ClassDef(DataBuffer,0)
  };

  //_____________________________________________________________________________
  inline
  void DataBuffer::Clear()
  {
    // Tell the buffer that it is supposed to be empty
    fNdata = 0;
  }

  //_____________________________________________________________________________
  inline
  void DataBuffer::SetSize( size_t nbytes )
  {
    // Tell the buffer its new size. Can be used after externally copying data
    // into the buffer.
    assert( nbytes <= fNalloc );
    fNdata = nbytes;
  }

} // namespace Podd

#endif
