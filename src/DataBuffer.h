#ifndef Podd_DataBuffer_h
#define Podd_DataBuffer_h

//////////////////////////////////////////////////////////////////////////
//
// Podd::DataBuffer
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"

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

    char*  Get() const { return fData; }
    size_t GetSize() const { return fNdata; }
    size_t GetAllocation() const { return fNalloc; }
    void   Clear();
    Int_t  Compact( size_t n = 0 );
    Int_t  Fill( const void* ptr, size_t nbytes, size_t i = 0 );
    Int_t  Resize( size_t n );

  private:
    char*   fData;    // Data buffer
    size_t  fNalloc;  // Bytes allocated
    size_t  fNdata;   // Bytes used

    static const size_t MAX = 100*1024*1024;  // sanity limit of 100 MB per buffer

    Int_t Realloc( size_t newsize );

    ClassDef(DataBuffer,0)
  };

} // namespace Podd

#endif
