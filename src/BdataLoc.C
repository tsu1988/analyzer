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
#include "THaGlobals.h"
#include "THaVarList.h"

#include <cstring>   // for memchr

using namespace std;

//_____________________________________________________________________________
BdataLoc::~BdataLoc()
{
  // Destructor - clean up global variable(s)

  // The following call will always invoke the base class instance 
  // BdataLoc::DefineVariables, even when destroying derived class
  // objects. But that's OK since the kDelete mode is the same for 
  // the entire class hierarchy; it simply removes the name.

  DefineVariables( THaAnalysisObject::kDelete );
}

//_____________________________________________________________________________
Int_t BdataLoc::DefineVariables( EMode mode )
{
  // Export this object's data as a global variable

  // Note that the apparatus prefix is already part of this object's name,
  // e.g. "D.syncroc1", where "D" is the name of the parent THaDecData object

  Int_t ret = THaAnalysisObject::kOK;

  switch( mode ) {
  case THaAnalysisObject::kDefine:
    {
      THaVar* var = gHaVars->Define( GetName(), data );
      if( !var ) {
	// Check if we are trying to redefine ourselves, if so, succeed with
	// warning (printed by Define() call above), else fail
	var = gHaVars->Find( GetName() );
	if( var == 0 || var->GetValuePointer() != &data ) {
	  ret = THaAnalysisObject::kInitError;
	}
      }
    }
    break;
  case THaAnalysisObject::kDelete:
    gHaVars->RemoveName( GetName() );
    break;
  }

  return ret;
}

//_____________________________________________________________________________
Int_t CrateLocMulti::DefineVariables( EMode mode )
{
  // For multivalued data (multihit modules), define a variable-sized global
  // variable on the vector<UInt_t> rdata member.

  //TODO: This doesn't work yet. We need support for STL containers in our
  // global variable system

  return CrateLoc::DefineVariables(mode);
}

//_____________________________________________________________________________
Int_t TrigBitLoc::DefineVariables( EMode mode )
{
  // Define the global variable for trigger bit test result. This is stored
  // in the "data" member of the base class, not in the rdata array, so here
  // we just do what the base class does.

  return BdataLoc::DefineVariables(mode);
}

//_____________________________________________________________________________
void CrateLoc::Load( const THaEvData& evdata )
{
  // Load one data word from crate/slot/chan address

  if( evdata.GetNumHits(crate,slot,chan) > 0 ) {
    data = evdata.GetData(crate,slot,chan,0);
  }
}

//_____________________________________________________________________________
void CrateLocMulti::Load( const THaEvData& evdata )
{
  // Load all decoded hits from crate/slot/chan address

  data = 0;
  for (Int_t i = 0; i < evdata.GetNumHits(crate,slot,chan); ++i) {
    rdata.push_back( evdata.GetData(crate,slot,chan,i) );
  }
}

//FIXME: This goes into the Hall A library
//_____________________________________________________________________________
void TrigBitLoc::Load( const THaEvData& evdata )
{
  // Test hit(s) in our TDC channel for a valid trigger bit and set results

  // Read hit(s) from defined multihit TDC channel
  CrateLocMulti::Load( evdata );

  // Figure out which triggers got a hit.  These are multihit TDCs, so we
  // need to sort out which hit we want to take by applying cuts.
  for( UInt_t ihit = 0; ihit < NumHits(); ++ihit ) {
    if( Get(ihit) > cutlo && Get(ihit) < cuthi ) {
      data = 1;
      if( bitloc )
	*bitloc |= BIT(bitnum);
      break;
    }
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
  // is no "memstr" in the standard library.

  //FIXME: Can this be made faster because each header is followed by the offset
  // to the next header?

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
ClassImp(TrigBitLoc)
ClassImp(WordLoc)
ClassImp(RoclenLoc)
