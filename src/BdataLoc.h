#ifndef PODD_BdataLoc
#define PODD_BdataLoc

//////////////////////////////////////////////////////////////////////////
//
// BdataLoc
//
// Utility class for THaDecData generic raw data decoder
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"

//FIXME: make this a nice OO hierarchy for different derived classes for
// the different data type (crate, header, etc.)
//FIXME: let this class handle its associated global analyzer variable
//FIXME: add "roclen" data type
class BdataLoc : public TNamed {
  // Utility class used by THaDecData.
  // Data location, either in (crates, slots, channel), or
  // relative to a unique header in a crate or in an event.
  static const Int_t MxHits=16;
public:
  // c'tor for (crate,slot,channel) selection
  BdataLoc ( const char* nm, Int_t cra, Int_t slo, Int_t cha ) :
    TNamed(nm,nm), crate(cra), slot(slo), chan(cha), header(0), ntoskip(0), 
    search_choice(0) { Clear(); }
  // c'tor for header search (note, the only diff to above is 3rd arg is UInt_t)
  //FIXME: this kind of overloading (Int/UInt) is asking for trouble...
  BdataLoc ( const char* nm, Int_t cra, UInt_t head, Int_t skip ) :
    TNamed(nm,nm), crate(cra), slot(0), chan(0), header(head), ntoskip(skip),
    search_choice(1) { Clear(); }
  Bool_t IsSlot() { return (search_choice == 0); }
  void Clear( const Option_t* ="" ) { ndata=0;  loaded_once = kFALSE; }
  void Load(UInt_t data) {
    if (ndata<MxHits) rdata[ndata++]=data;
    loaded_once = kTRUE;
  }
  Bool_t DidLoad() { return loaded_once; }
  Int_t NumHits() { return ndata; }
  //FIXME: not really needed if the global analyzer variable is right in here
  UInt_t Get(Int_t i=0) { 
    return (i >= 0 && ndata > i) ? rdata[i] : 0; }
  Bool_t operator==( const char* aname ) { return fName==aname; }
  // operator== and != compare the hardware definitions of two BdataLoc's
  Bool_t operator==( const BdataLoc& rhs ) const {
    return ( search_choice == rhs.search_choice && crate == rhs.crate &&
	     ( (search_choice == 0 && slot == rhs.slot && chan == rhs.chan) ||
	       (search_choice == 1 && header == rhs.header && 
		ntoskip == rhs.ntoskip)
	       )  
	     );
  }
  Bool_t operator!=( const BdataLoc& rhs ) const { return !(*this == rhs ); }
  //FIXME: Set functions not needed if we put the database line parser in here
  void SetSlot( Int_t cr, Int_t sl, Int_t ch ) {
    crate = cr; slot = sl; chan = ch; header = ntoskip = search_choice = 0;
  }
  void SetHeader( Int_t cr, UInt_t hd, Int_t sk ) {
    crate = cr; header = hd; ntoskip = sk; slot = chan = 0; search_choice = 1;
  }
  ~BdataLoc() {}
   
  Int_t  crate, slot, chan;   // where to look in crates
  UInt_t header;              // header (unique either in data or in crate)
  Int_t ntoskip;              // how far to skip beyond header
   
  //FIXME: each subclass of BdataLoc should support its own data type here.
  // Not all hardware is/needs multihit capability. Should support single-value
  // data as well.
  UInt_t rdata[MxHits];       //[ndata] raw data (to accom. multihit chanl)
  Int_t  ndata;               // number of relevant entries
private:
  Int_t  search_choice;       // whether to search in crates or rel. to header
  Bool_t loaded_once;
  BdataLoc();
  BdataLoc(const BdataLoc& dataloc);
  BdataLoc& operator=(const BdataLoc& dataloc);

  ClassDef(BdataLoc,0)  
};

#endif
