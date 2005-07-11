#ifndef THaVDCSimDecoder_
#define THaVDCSimDecoder_

/////////////////////////////////////////////////////////////////////
//
//   THaVDCSimDecoder
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "TClonesArray.h"

class THaCrateMap;
class TList;

class THaVDCSimDecoder : public THaEvData {
 public:
  THaVDCSimDecoder();
  virtual ~THaVDCSimDecoder();

  Int_t LoadEvent( const int*evbuffer, THaCrateMap* usermap );

  void Clear( Option_t* opt="" );
  Int_t GetNTracks() const;

 protected:

  TList*  fTracks;    // Monte Carlo tracks


  ClassDef(THaVDCSimDecoder,0) // Decoder for simulated VDC data
};

#endif
