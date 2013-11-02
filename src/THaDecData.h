#ifndef ROOT_THaDecData
#define ROOT_THaDecData

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "THashList.h"

// TODO:
// - Use TList* for fBdataLoc?
// - override more THaApparatus functions?
// - IMPORTANT: how to make global variables/output survive if reinitializing?

class THaDecData : public THaApparatus {
  
public:
   THaDecData( const char* name,
	       const char* description = "Raw decoder data" );
   virtual ~THaDecData();

   virtual EStatus Init( const TDatime& run_time );
   virtual void    Clear( Option_t* opt="" );
   virtual Int_t   Decode( const THaEvData& );
   virtual void    Print( Option_t* opt="" ) const;
   virtual Int_t   Reconstruct() { return 0; }

protected:

  UInt_t     evtype;      // CODA event type
  UInt_t     evtypebits;  // Bitpattern of active trigger numbers
  THashList  fBdataLoc;   // Raw data channels

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaDecData,0)
};

#endif
