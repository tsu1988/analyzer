#ifndef ROOT_THaScCalib
#define ROOT_THaScCalib


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScCalib                                                                //
//    Interface to test out and attempt to calibrate the Scintillator        //
//    detectors. Provides access to the correction factors and the           //
//    data.                                                                  //
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include "THaScintillator.h"

class THaScCalib {
 public:
  THaScCalib(THaScintillator* s);
  
 private:
  THaScintillator* fSc;

  Double_t  zero;
 public:
  enum ESide { LeftPMT=0, RightPMT=1 };
  Double_t& TDC(Int_t pad, ESide side);
  Double_t& ADC(Int_t pad, ESide side);

  Double_t& PMTTime(Int_t pad, ESide side);
  Double_t& ADC_C(Int_t pad, ESide side);

  void     ClearEvent() { fSc->ClearEvent(); }
  Int_t    ReadAsciEvent(istream& in);

  Int_t    NPaddles() { return fSc->fNelem; }
  Double_t& TDCOffset(Int_t pad, ESide side );
  Double_t& ADCPedestal(Int_t pad, ESide side );
  Double_t& ADCGain(Int_t pad, ESide side );

  // could be modified to permit non-linearities
  Double_t  TDC2Time(Double_t tdc) { return tdc*fSc->fTdc2T ; }
  Double_t  TDCRes() { return fSc->fTdc2T; }
  Int_t    NTimeWalkP() { return fSc->fNTWalkPar; }
  Int_t    SetTimeWalkP(Int_t n, Double_t* par);

  Double_t& TimeWalkP(Int_t p) { return fSc->fTWalkPar[p]; }
  Double_t& PropSpeed() { return fSc->fCn; }

  Double_t& TR_Offset(Int_t pad);
  THaScintillator* Scintillator() { return fSc; }

  void WriteDB(const TDatime& date);
};


#endif
