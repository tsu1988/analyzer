//*-- Author :    Ole Hansen   27-Apr-04

//////////////////////////////////////////////////////////////////////////
//
// BeamInfo
//
// Utility class/structure for holding variable beam information.
//
//////////////////////////////////////////////////////////////////////////

#include "BeamInfo.h"
#include "Beam.h"
#include "THaRunParameters.h"
#include "TMath.h"

using namespace std;

namespace Podd {

const Double_t BeamInfo::kBig = 1e38;

//_____________________________________________________________________________
Double_t BeamInfo::GetE() const
{
  // Return beam energy. This is the energy corresponding to the
  // current beam momentum and mass, not necessarily the same as
  // the nominal beam energy in the run database.

  if( !fBeam )
    return kBig;

  Double_t m = GetM();
  Double_t p = GetP();
  return TMath::Sqrt( p*p + m*m );
}

//_____________________________________________________________________________
Double_t BeamInfo::GetM() const
{
  // Return mass of beam particles

  if( !fBeam )
    return kBig;
  THaRunParameters* rp = fBeam->GetRunParameters();
  if( !rp )
    return kBig;
  return rp->GetBeamM();
}

//_____________________________________________________________________________
Int_t BeamInfo::GetQ() const
{
  // Return charge of beam particles (electrons: -1)

  if( !fBeam )
    return 0;
  THaRunParameters* rp = fBeam->GetRunParameters();
  if( !rp )
    return 0;
  return rp->GetBeamQ();
}

//_____________________________________________________________________________
Double_t BeamInfo::GetdE() const
{
  // Return beam energy uncertainty

  if( !fBeam )
    return kBig;
  THaRunParameters* rp = fBeam->GetRunParameters();
  if( !rp )
    return kBig;
  return rp->GetBeamdE();
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

ClassImp(Podd::BeamInfo)
