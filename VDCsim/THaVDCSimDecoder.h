#ifndef THaVDCSimDecoder_
#define THaVDCSimDecoder_

/////////////////////////////////////////////////////////////////////
//
//   THaVDCSimDecoder
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include "evio.h"

class THaBenchmark;
class THaCrateMap;
class THaHelicity;

class THaVDCSimDecoder : public THaEvData {
 public:
  THaVDCSimDecoder();
  ~THaVDCSimDecoder();

  int LoadEvent(const int*evbuffer, THaCrateMap* usermap);

 protected:

  ClassDef(THaVDCSimDecoder,0) // Decoder for simulated VDC data
};

#endif
