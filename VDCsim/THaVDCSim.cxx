#include "THaVDCSim.h"

ClassImp(THaVDCSimWireHit);
ClassImp(THaVDCSimEvent);
ClassImp(THaVDCSimConditions);

THaVDCSimEvent::THaVDCSimEvent() {
  for (int i = 0; i < 4; i++)
    wirehits[i] = new TList;
}

THaVDCSimEvent::~THaVDCSimEvent() {
  Clear();
  for (int i = 0; i < 4; i++)
    delete wirehits[i];
}

void THaVDCSimEvent::Clear( const Option_t* opt ) {
  for (Int_t i = 0; i < 4; i++)
    wirehits[i]->Delete();
}

void THaVDCSimConditions::set(Float_t *something,
			      Float_t a, Float_t b, Float_t c, Float_t d) {
  something[0] = a;
  something[1] = b;
  something[2] = c;
  something[3] = d;
}
