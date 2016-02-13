//*-- Author :    Ole Hansen   02-Feb-2016


//////////////////////////////////////////////////////////////////////////
//
// Podd::DataBuffer
//
// A generic, auto-resizing buffer for arbitrary data. The caller must
// interpret the data as appropriate.
//
// Primarily used as buffer for filling TBranches in THaOutput
//
//////////////////////////////////////////////////////////////////////////

#include "DataBuffer.h"
#include <cstring>   // for memcpy
#include <cassert>
#include <utility>

using namespace std;

namespace Podd {

  //_____________________________________________________________________________
  DataBuffer::DataBuffer( size_t n /* = 0 */ )
    : fData(0), fNalloc(0), fNdata(0)
  {
    // Constructor. Creates an empty object (buffer size zero) by default.
    // If n > 0, allocate n bytes
    if( n > 0 )
      Resize(n);
  }

  //_____________________________________________________________________________
  DataBuffer::DataBuffer( const DataBuffer& rhs )
    : fData(0), fNalloc(rhs.fNalloc), fNdata(rhs.fNdata)
  {
    // Copy constructor
    assert( fNalloc <= MAX );
    if( fNalloc > MAX ) fNalloc = MAX;
    assert( fNalloc >= fNdata );
    if( fNdata > fNalloc ) fNdata = fNalloc;
    if( fNalloc > 0 ) {
      fData = new char[fNalloc];
      memcpy( fData, rhs.fData, fNdata);
    }
  }

  //_____________________________________________________________________________
  DataBuffer& DataBuffer::operator=( const DataBuffer& rhs)
  {
    // Assignment operator
    if( this != &rhs ) {
      fNalloc = rhs.fNalloc;
      fNdata  = rhs.fNdata;
      assert( fNalloc <= MAX );
      if( fNalloc > MAX ) fNalloc = MAX;
      assert( fNalloc >= fNdata );
      if( fNdata > fNalloc ) fNdata = fNalloc;
      delete [] fData; fData = 0;
      if( fNalloc > 0 ) {
	fData = new char[fNalloc];
	memcpy( fData, rhs.fData, fNdata);
      } else
	fData = 0;
    }
    return *this;
  }

#if __cplusplus >= 201103L
  //_____________________________________________________________________________
  DataBuffer::DataBuffer( DataBuffer&& rhs )
    : fData(rhs.fData), fNalloc(rhs.fNalloc), fNdata(rhs.fNdata)
  {
    // Move constructor
    rhs.fNalloc = rhs.fNdata = 0;
    rhs.fData = nullptr;
  }

  //_____________________________________________________________________________
  DataBuffer& DataBuffer::operator=( DataBuffer&& rhs)
  {
    // Move assignment
    if( this != &rhs ) {
      swap( fData,   rhs.fData );
      swap( fNalloc, rhs.fNalloc );
      swap( fNdata,  rhs.fNdata );
    }
    return *this;
  }
#endif

  //_____________________________________________________________________________
  DataBuffer::~DataBuffer()
  {
    // Destructor
    delete [] fData;
  }

  //_____________________________________________________________________________
  void DataBuffer::Clear()
  {
    // Tell the buffer that it is supposed to be empty
    fNdata = 0;
  }

  //_____________________________________________________________________________
  Int_t DataBuffer::Fill( const void* ptr, size_t nbytes, size_t i /* = 0 */ )
  {
    // Copy nbytes from ptr to buffer. If i is non-zero, write to element i
    // of the buffer, assumed to be an array of some data type with size nbytes.
    size_t n = (i+1)*nbytes;
    if( n > fNalloc && !Resize(n) ) return 1;
    if( n > fNdata ) fNdata = n;
    memcpy( fData+i*nbytes, ptr, nbytes );
    return 0;
  }

  //_____________________________________________________________________________
  Int_t DataBuffer::Compact( size_t n /* = 0 */ )
  {
    // Resize buffer to currently used size (shrink to fit). To release all
    // memory, do Clear() first. This will reallocate the buffer unless
    // the new size is exatly equal to the old size.
    if( n == 0 ) n = fNdata;
    if( fNalloc == n ) return 0;
    return Realloc(n);
  }

  //_____________________________________________________________________________
  Int_t DataBuffer::Realloc( size_t newsize )
  {
    // Reallocate the buffer to newsize bytes. Copy as much used data from the old
    // buffer to the new one as will fit.
    char* tmp = 0;
    if( newsize > 0 ) {
      tmp = new char[newsize];
      if( fNdata > newsize ) fNdata = newsize;
      memcpy( tmp, fData, fNdata );
    }
    delete [] fData;
    fData = tmp;
    fNalloc = newsize;
    return 0;
  }

  //_____________________________________________________________________________
  Int_t DataBuffer::Resize( size_t n )
  {
    // Make sure the buffer can hold at least n bytes. Reallocates the buffer
    // if necessary, so Get() may return a different pointer after this call.

    if( n <= fNalloc ) return 0;
    if( n > MAX ) return 1;
    size_t newsize = fNalloc;
    if( newsize == 0 ) newsize = 1;
    while( n > newsize ) { newsize *= 2; }
    if( newsize > MAX ) newsize = MAX;
    return Realloc(newsize);
  }

} // namespace Podd

//_____________________________________________________________________________
ClassImp(Podd::DataBuffer);
