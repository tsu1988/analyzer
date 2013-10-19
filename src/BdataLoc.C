//*-- Author :    Bob Michaels,  March 2002

//////////////////////////////////////////////////////////////////////////
//
// BdataLoc, CrateLoc, WordLoc
//
// Utility classes for THaDecData generic raw data decoder
//
//////////////////////////////////////////////////////////////////////////

#include "BdataLoc.h"
#include "THaEvData.h"
#include <cstring>   // for memchr
#include <cassert>

using namespace std;

const Double_t BdataLoc::kBig = 1.e38;

//_____________________________________________________________________________
void CrateLoc::Load( const THaEvData& evdata )
{
  // Load decoded data from crate/slot/chan address

  if( evdata.GetNumHits(crate,slot,chan) > 0 ) {
    data = evdata.GetData(crate,slot,chan,0);
  }
}

//_____________________________________________________________________________
void CrateLocMulti::Load( const THaEvData& evdata )
{
  // Load all decoded hits from crate/slot/chan address

  for (Int_t i = 0; i < evdata.GetNumHits(crate,slot,chan); ++i) {
    rdata.push_back( evdata.GetData(crate,slot,chan,i) );
  }
}

//_____________________________________________________________________________
void WordLoc::Load( const THaEvData& evdata )
{
  // Load data at header/notoskip position from crate data buffer 

  typedef const Int_t rawdata_t;

  Int_t roclen = evdata.GetRocLength(crate);
  if( roclen < ntoskip+1 ) return;

  rawdata_t* cratebuf = evdata.GetRawDataBuffer(crate), *endp = cratebuf+roclen;
  assert(cratebuf);  // Must exist if roclen > 0

  // Accelerated search for the header word. Coded explicitly because there
  // is "memstr" in the standard library.

  // Get the first byte of the header, regardless of byte order
  int h = ((UChar_t*)&header)[0];
  register rawdata_t* p = cratebuf;
  while( (p = (rawdata_t*)memchr(p,h,sizeof(rawdata_t)*(endp-p-1)+1)) && 
	 p <= endp-ntoskip-1 ) {
    // The header must be aligned at a word boundary
    int off = (p-cratebuf) & (sizeof(rawdata_t)-1);  // same as % sizeof()
    if( off != 0 ) {
      p += sizeof(rawdata_t)-off;
      continue;
    }
    // Compare all 4 bytes of the header word
    if( memcmp(p,&header,sizeof(rawdata_t)) == 0 ) {
      // Fetch the requested word (as UInt_t, i.e. 4 bytes)
      // BTW, notoskip == 0 makes no sense since it would fetch the header itself
      data = *(p+ntoskip);
      break;
    }
    p += sizeof(rawdata_t);
  }
}

//_____________________________________________________________________________
void RoclenLoc::Load( const THaEvData& evdata )
{
  // Get event length for this crate

  data = evdata.GetRocLength(crate);
}

//_____________________________________________________________________________
ClassImp(BdataLoc)
ClassImp(CrateLoc)
ClassImp(CrateLocMulti)
ClassImp(WordLoc)
ClassImp(RoclenLoc)
