#ifndef ROOT_DecData
#define ROOT_DecData

//////////////////////////////////////////////////////////////////////////
//
// DecData
//
//////////////////////////////////////////////////////////////////////////

#include "Apparatus.h"
#include "THashList.h"
#include "BdataLoc.h"

class TString;

namespace Podd {

class DecData : public Apparatus {

public:
  DecData( const char* name = "D",
	   const char* description = "Raw decoder data" );
  virtual ~DecData();

  virtual EStatus Init( const TDatime& run_time );
  virtual void    Clear( Option_t* opt="" );
  virtual Int_t   Decode( const THaEvData& );
  virtual void    Print( Option_t* opt="" ) const;

  // Disabled functions from Apparatus
  virtual Int_t   AddDetector( Detector* det ) { return 0; }
  virtual Int_t   Reconstruct() { return 0; }

protected:
  UInt_t          evtype;      // CODA event type
  UInt_t          evtypebits;  // Bitpattern of active trigger numbers
  THashList       fBdataLoc;   // Raw data channels

  virtual Int_t   DefineVariables( EMode mode = kDefine );
  virtual FILE*   OpenFile( const TDatime& date );
  virtual Int_t   ReadDatabase( const TDatime& date );

  Int_t           DefineLocType( const BdataLoc::BdataLocType& loctype,
				 const TString& configstr, bool re_init );

  ClassDef(DecData,0)
};

} // end namespace Podd

// Backwards-compatibility
#define THaDecData Podd::DecData

#endif
