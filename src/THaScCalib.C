
#include "THaScCalib.h"

#include "THaDB.h"

THaScCalib::THaScCalib(THaScintillator* sc): fSc(sc) { }

Int_t THaScCalib::ReadAsciEvent(istream& in) { return 0; }
// a dummy routine for now

Double_t& THaScCalib::TDC(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLT[pad];
  }

  return fSc->fRT[pad];
}

Double_t& THaScCalib::PMTTime(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLT_c[pad];
  }

  return fSc->fRT_c[pad];
}

Double_t& THaScCalib::ADC(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLA[pad];
  }

  return fSc->fRA[pad];
}

Double_t& THaScCalib::ADC_C(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLA_c[pad];
  }

  return fSc->fRA_c[pad];
}

Double_t& THaScCalib::TDCOffset(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLOff[pad];
  }
  return fSc->fROff[pad];
}

Double_t& THaScCalib::ADCPedestal(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLPed[pad];
  }

  return fSc->fRPed[pad];
}

Double_t& THaScCalib::ADCGain(Int_t pad, ESide side) {
  if (side == LeftPMT) {
    return fSc->fLGain[pad];
  }

  return fSc->fRGain[pad];
}

Int_t THaScCalib::SetTimeWalkP(Int_t n, Double_t* par) {
  if (n != fSc->fNTWalkPar) {
    delete [] fSc->fTWalkPar;
    fSc->fTWalkPar = new Double_t[n];
    fSc->fNTWalkPar = n;
  }
  for (Int_t i=0; i<n; i++) {
    fSc->fTWalkPar[i] = par[i];
  }
  return n;
}

Double_t& THaScCalib::TR_Offset(Int_t pad) {
  return fSc->fTrigOff[pad];
}

void THaScCalib::WriteDB(const TDatime& date) {
  if ( ! gHaDB ) {
    return;
  }

  Int_t n = fSc->fNelem;

  const TagDef list[] = {
    { "TDC_offsetsL", fSc->fLOff, 0, n },
    { "TDC_offsetsR", fSc->fROff, 0, n },
    { "ADC_pedsL", fSc->fLPed, 0, n },
    { "ADC_pedsR", fSc->fRPed, 0, n },
    { "ADC_coefL", fSc->fLGain, 0, n },
    { "ADC_coefR", fSc->fRGain, 0, n },
    { "TDC_res",   &fSc->fTdc2T, 0 },
    { "TransSpd",  &fSc->fCn,  0 },
    { "AdcMIP",    &fSc->fAdcMIP, 0},
    { "NTWalk",    &fSc->fNTWalkPar, 0, 0, kInt },
    { "Timewalk",  fSc->fTWalkPar, 0, fSc->fNTWalkPar },
    { 0 }
  };
  
  gHaDB->StoreValues(fSc->GetPrefix(), list, date);
}
