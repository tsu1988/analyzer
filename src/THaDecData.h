#ifndef ROOT_THaDecData
#define ROOT_THaDecData

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include <vector>

class BdataLoc;

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

  std::vector<BdataLoc*> fBdataLoc;  // Raw data locations

  virtual Int_t DefineVariables( EMode mode = kDefine );

  UInt_t evtype;
  UInt_t evtypebits;

  // ============= OLD ============
#if 0
   virtual BdataLoc* DefineChannel(BdataLoc*, EMode, const char* desc="automatic");

   Double_t ctimel, ctimer;
   Double_t pulser1;
   UInt_t synchadc1, synchadc2, synchadc3, 
          synchadc4, synchadc14;
   UInt_t timestamp, timeroc1, timeroc2, timeroc3,  
          timeroc4, timeroc14;
   Double_t rftime1,rftime2;
   Double_t edtpl,edtpr;
   Double_t lenroc12,lenroc16;
  //FIXME: combine into one collection for better efficiency - and decide on the
  // proper data structure (array vs. list/hashlist etc.)
   std::vector < BdataLoc* > fCrateLoc;   // Raw Data locations by crate, slot, channel
   std::vector < BdataLoc* > fWordLoc;    // Raw Data locations relative to header word

   std::vector<TH1F* > hist;
   Int_t DefaultMap();
   void TrigBits(UInt_t ibit, BdataLoc *dataloc);
   Int_t SetupDecData( const TDatime* runTime = NULL, EMode mode = kDefine );
   virtual void BookHist(); 

  static THaDecData* fgThis;   //Pointer to instance of this class
#endif

  ClassDef(THaDecData,0)  
};

#endif
