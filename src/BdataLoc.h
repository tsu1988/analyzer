#ifndef PODD_BdataLoc
#define PODD_BdataLoc

//////////////////////////////////////////////////////////////////////////
//
// BdataLoc
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include <vector>

class THaEvData;

//FIXME: let this class handle its associated global analyzer variable
//___________________________________________________________________________
class BdataLoc : public THaAnalysisObject {
  // Utility class used by THaDecData.
  // Data location, either in (crates, slots, channel), or
  // relative to a unique header in a crate or in an event.
public:
  // Base class constructor
  BdataLoc( const char* name, Int_t cra )
    : THaAnalysisObject(name,name), crate(cra), data(kBig) { }
  BdataLoc() {}  // For ROOT RTTI only
  virtual ~BdataLoc() {}

  // Main function: extract the defined data from the event
  virtual void    Load( const THaEvData& evt ) = 0;

  virtual void    Clear( const Option_t* ="" )  { data = kBig; }
  virtual Bool_t  DidLoad() const               { return (data != kBig); }
  virtual UInt_t  NumHits() const               { return DidLoad() ? 1 : 0; }
  virtual UInt_t  Get( Int_t i = 0 ) const      { return data; }
  Bool_t operator==( const char* aname ) const  { return fName == aname; }
  // operator== and != compare the hardware definitions of two BdataLoc's
  // virtual Bool_t operator==( const BdataLoc& rhs ) const
  // { return (crate == rhs.crate); }
  // Bool_t operator!=( const BdataLoc& rhs ) const { return !(*this==rhs); }
   
protected:
  Int_t   crate;   // Crate where these data originate
  UInt_t  data;    // raw data word

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(BdataLoc,0)  
};

//___________________________________________________________________________
class CrateLoc : public BdataLoc {
public:
  // c'tor for (crate,slot,channel) selection
  CrateLoc( const char* nm, Int_t cra, Int_t slo, Int_t cha )
    : BdataLoc(nm,cra), slot(slo), chan(cha) { }
  virtual ~CrateLoc() {}

  virtual void   Load( const THaEvData& evt );

  // virtual Bool_t operator==( const BdataLoc& rhs ) const
  // { return (crate == rhs.crate && slot == rhs.slot && chan == rhs.chan); }

protected:
  Int_t slot, chan;    // Data location

  ClassDef(CrateLoc,0)  
};

//___________________________________________________________________________
class CrateLocMulti : public CrateLoc {
public:
  // (crate,slot,channel) allowing for multiple hits per channel
  CrateLocMulti( const char* nm, Int_t cra, Int_t slo, Int_t cha )
    : CrateLoc(nm,cra,slo,cha) { }
  virtual ~CrateLocMulti() {}

  virtual void    Load( const THaEvData& evt );

  virtual void    Clear( const Option_t* ="" )  { CrateLoc::Clear(); rdata.clear(); }
  virtual UInt_t  NumHits() const               { return rdata.size(); }
  virtual UInt_t  Get( Int_t i = 0 ) const      { return rdata.at(i); }

protected:
  std::vector<UInt_t> rdata;     // raw data

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(CrateLocMulti,0)  
};

//FIXME: put into Hall A library
//___________________________________________________________________________
class TrigBitLoc : public CrateLocMulti {
public:
  // (crate,slot,channel) multihit TDC for Hall A-style trigger bits
  TrigBitLoc( const char* nm, Int_t cra, Int_t slo, Int_t cha,
	      UInt_t num, UInt_t lo, UInt_t hi, UInt_t* loc )
    : CrateLocMulti(nm,cra,slo,cha), bitnum(num), cutlo(lo), cuthi(hi),
      bitloc(loc) { }
  virtual ~TrigBitLoc() {}

  virtual void    Load( const THaEvData& evt );

protected:
  UInt_t  bitnum;        // Bit number for this variable
  UInt_t  cutlo, cuthi;  // TDC cut for detecting valid trigger bit data
  UInt_t* bitloc;        // External bitpattern variable to fill

  // Re-uses base class "data" member for result of per-event bit test
  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(TrigBitLoc,0)  
};

//___________________________________________________________________________
class WordLoc : public BdataLoc {
public:
  // c'tor for header search 
  WordLoc( const char* nm, Int_t cra, UInt_t head, Int_t skip )
    : BdataLoc(nm,cra), header(head), ntoskip(skip) { }
  virtual ~WordLoc() {}

  virtual void   Load( const THaEvData& evt );

  // virtual Bool_t operator==( const BdataLoc& rhs ) const
  // { return (crate == rhs.crate &&
  // 	    header == rhs.header && ntoskip == rhs.ntoskip); }

protected:
  UInt_t header;              // header (unique either in data or in crate)
  Int_t  ntoskip;             // how far to skip beyond header
   
  ClassDef(WordLoc,0)  
};

//___________________________________________________________________________
class RoclenLoc : public BdataLoc {
public:
  // Event length of a crate
  RoclenLoc( const char* nm, Int_t cra ) : BdataLoc(nm,cra) { }
  virtual ~RoclenLoc() {}

  virtual void   Load( const THaEvData& evt );

  ClassDef(RoclenLoc,0)  
};

//======== OLD ===========
#if 0
  Bool_t IsSlot() { return (search_choice == 0); }
  void SetSlot( Int_t cr, Int_t sl, Int_t ch ) {
    crate = cr; slot = sl; chan = ch; header = ntoskip = search_choice = 0;
  }
  void SetHeader( Int_t cr, UInt_t hd, Int_t sk ) {
    crate = cr; header = hd; ntoskip = sk; slot = chan = 0; search_choice = 1;
  }

void    Load( UInt_t data)            { rdata.push_back(data); }

( search_choice == rhs.search_choice && crate == rhs.crate &&
	     ( (search_choice == 0 && slot == rhs.slot && chan == rhs.chan) ||
	       (search_choice == 1 && header == rhs.header && 
		ntoskip == rhs.ntoskip)
	       )  
	     );
  }


  Int_t  search_choice;       // whether to search in crates or rel. to header
#endif

///////////////////////////////////////////////////////////////////////////////

#endif
