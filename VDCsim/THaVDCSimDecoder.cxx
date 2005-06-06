/////////////////////////////////////////////////////////////////////
//
//   THaVDCSimDecoder
//   Hall A VDC Event Data from a predefined ROOT file
//
//   author  Ken Rossato (rossato@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaVDCSimDecoder.h"
#include "THaVDCSim.h"
#include "THaHelicity.h"
#include "THaCrateMap.h"
#include "THaUsrstrutils.h"
#include "THaBenchmark.h"
#include "TError.h"
#include <cstring>
#include <cstdio>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <ctime>

using namespace std;

static const int VERBOSE = 1;
static const int DEBUG   = 0;
static const int BENCH   = 0;

THaVDCSimDecoder::THaVDCSimDecoder() {

}

THaVDCSimDecoder::~THaVDCSimDecoder() {

}

int THaVDCSimDecoder::LoadEvent(const int*evbuffer, THaCrateMap* map) {
  int status = HED_OK;
  fMap = map;

  // KCR- don't forget to allocate "buffer" at some point
  buffer = 0;

  const THaVDCSimEvent *simEvent = 
    reinterpret_cast<const THaVDCSimEvent*>(evbuffer);

  if(DEBUG) PrintOut();
  if (first_decode) {
    init_cmap();     
    if (init_slotdata(map) == HED_ERR) return HED_ERR;
    first_decode = false;
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  for( int i=0; i<fNSlotClear; i++ )
    crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  
  evscaler = 0;

  // FIXME -KCR, put it off, fill it once we know what the answer is
  // If we don't fix the other section below, we can leave this as is
  event_length = 0;
  //

  event_type = 1;
  event_num = simEvent->event_num;
  recent_event = event_num;

  //KCR: Begin code from "physics_decode" function 

  if( fDoBench ) fBench->Begin("physics_decode");

  //KCR: fixme? This section can only exist if we fill evbuffer
  //     Without this section we break two GetRawData functions
  /*
  memset(rocdat,0,MAXROC*sizeof(RocDat_t));
     // n1 = pointer to first word of ROC
     int pos = evbuffer[2]+3;
     int nroc = 0;
     int irn[MAXROC];   // Lookup table i-th ROC found -> ROC number
     while( pos+1 < event_length && nroc < MAXROC ) {
       int len  = evbuffer[pos];
       int iroc = (evbuffer[pos+1]&0xff0000)>>16;
       if(iroc>=MAXROC) {
         if(VERBOSE) { 
  	   cout << "ERROR in THaVDCSimDecoder::physics_decode:";
	   cout << "  illegal ROC number " <<dec<<iroc<<endl;
	 }
	 if( fDoBench ) fBench->Stop("physics_decode");
         return HED_ERR;
       }
// Save position and length of each found ROC data block

// KCR: BINGO!  This is what I need to do:
//  update this rocdat guy, and populate an "evbuffer" with data, last
//  three hex digits indicating TDC values (+ offsets)
       rocdat[iroc].pos  = pos;
       rocdat[iroc].len  = len;
       irn[nroc++] = iroc;
       pos += len+1;
     }
  */
     if( fDoBench ) fBench->Stop("physics_decode");
// Decode each ROC
// This is not part of the loop above because it may exit prematurely due 
// to errors, which would leave the rocdat[] array incomplete.
/*     for( int i=0; i<nroc; i++ ) {
       int iroc = irn[i];
       const RocDat_t* proc = rocdat+iroc;
       int ipt = proc->pos + 1;
       int iptmax = proc->pos + proc->len;
*/
     // KCR: Now to populate the crateslot guy
     for (int i = 0; i < 4; i++) { // for each plane
       TObjLink *lnk = simEvent->wirehits[i]->FirstLink();
       while (lnk) { // iterate over hits
	 THaVDCSimWireHit *hit = static_cast<THaVDCSimWireHit*>
	   (lnk->GetObject());
	 
	 // KCR: HardCode crate/slot/chan nums:
	 Int_t chan = hit->wirenum % 96;
	 Int_t raw = static_cast<Int_t>(hit->time);
	 Int_t roc = 3;
	 Int_t slot = hit->wirenum / 96 + 6 + i*4;

	 if (crateslot[idx(roc,slot)]->loadData("tdc",chan,raw,raw)
	     == SD_ERR) return HED_ERR;
	 lnk = lnk->Next();
       }
     }

     // We aren't capable of providing Helicity data
     /*
     if( HelicityEnabled()) {
       if( !helicity ) {
	 helicity = new THaHelicity;
	 if( !helicity ) return HED_ERR;
       }
       if(helicity->Decode(*this) != 1) return HED_ERR;
       dhel = (Double_t)helicity->GetHelicity();
       dtimestamp = (Double_t)helicity->GetTime();
     }
     */
     return HED_OK;
}

ClassImp(THaVDCSimDecoder)
