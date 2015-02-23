#ifndef CodaDecoder_
#define CodaDecoder_

/////////////////////////////////////////////////////////////////////
//
//   CodaDecoder
//
//           Object Oriented version of decoder
//           Sept, 2014    R. Michaels
//
/////////////////////////////////////////////////////////////////////

#define MAX_EVTYPES 200
#define MAX_PHYS_EVTYPES 14

#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include "Decoder.h"
#include "THaEvData.h"
#include <vector>

class Decoder::CodaDecoder : public THaEvData {
  // public interface is SAME as before
 public:
  CodaDecoder();
  ~CodaDecoder();
 
  virtual Int_t LoadEvent(const UInt_t* evbuffer);    

  virtual Int_t GetNslots() { return fNSlotUsed; };

  virtual Int_t GetPrescaleFactor(Int_t trigger) const;
  
  virtual Int_t LoadIfFlagData(const UInt_t *p);

  virtual void SetRunTime(ULong64_t tloc);

  Int_t FindRocs(const UInt_t *evbuffer);
  Int_t roc_decode( Int_t roc, const UInt_t* evbuffer, Int_t ipt, Int_t istop );
  Int_t nroc;
  Int_t *irn;

 protected:

  Int_t   synchflag,datascan;
  Bool_t  buffmode,synchmiss,synchextra;

  Int_t *fbfound;

  void CompareRocs();
  void ChkFbSlot( Int_t roc, const UInt_t* evbuffer, Int_t ipt, Int_t istop );
  void ChkFbSlots();
  void FindUsedSlots();  

  int init_slotdata(const THaCrateMap *map);
  void dump(const UInt_t* evbuffer);

  ClassDef(CodaDecoder,0) // Decoder for CODA event buffer
};

#endif
