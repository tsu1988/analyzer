///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCHit                                                                 //
//                                                                           //
// Class representing a single hit for the VDC                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCHit.h"
#include "THaVDCTimeToDistConv.h"
#include <cassert>

const Double_t THaVDCHit::kBig = 1.e38; // Arbitrary large value

//_____________________________________________________________________________
Double_t THaVDCHit::ConvertTimeToDist( Double_t slope, Double_t t0 )
{
  // Converts TDC time to drift distance
  // Takes the (estimated) slope and time offset of the track as arguments

  assert( fWire->GetTTDConv() );  // set up in VDCPlane::ReadDatabase

  fDist = fWire->GetTTDConv()->ConvertTimeToDist( fTime-t0, slope, &fdDist );
  return fDist;
}

//_____________________________________________________________________________
Int_t THaVDCHit::Compare( const TObject* obj ) const
{
  // Used to sort hits
  // A hit is "less than" another hit if it occurred on a lower wire number.
  // Also, for hits on the same wire, the first hit on the wire (the one with
  // the smallest time) is "less than" one with a higher time.  If the hits
  // are sorted according to this scheme, they will be in order of increasing
  // wire number and, for each wire, will be in the order in which they hit
  // the wire

  assert( obj && IsA() == obj->IsA() );

  if( obj == this )
    return 0;

  assert( dynamic_cast<const THaVDCHit*>(obj) );
  const THaVDCHit* other = static_cast<const THaVDCHit*>( obj );

  ByWireThenTime isless;
  if( isless( this, other ) )
    return -1;
  if( isless( other, this ) )
    return 1;
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaVDCHit)
