//*-- Author :    Ole Hansen 25-Mar-2003

//////////////////////////////////////////////////////////////////////////
//
// Beam
//
// Abstract apparatus class that provides position and direction of
// a particle beam, usually event by event.
//
//////////////////////////////////////////////////////////////////////////

#include "Beam.h"
#include "VarDef.h"
#include "THaRunBase.h"
#include "THaRunParameters.h"
#include "THaGlobals.h"

namespace Podd {

//_____________________________________________________________________________
Beam::Beam( const char* name, const char* desc ) :
  Apparatus( name,desc )
{
  // Constructor.
  // Protected. Can only be called by derived classes.

  fBeamIfo.SetBeam( this );
}


//_____________________________________________________________________________
Beam::~Beam()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
AnalysisObject::EStatus Beam::Init( const TDatime& run_time )
{
  // Init method for a beam apparatus. Calls the standard Apparatus
  // initialization and, in addition, finds pointer to the current
  // run parameters.

  if( !gHaRun || !gHaRun->IsInit() ) {
    Error( Here("Init"), "Current run not initialized. "
	   "Failed to initialize beam apparatus %s (\"%s\"). ",
	   GetName(), GetTitle() );
    return fStatus = kInitError;
  }
  fRunParam = gHaRun->GetParameters();
  if( !fRunParam ) {
    Error( Here("Init"), "Current run has no parameters?!? "
	   "Failed to initialize beam apparatus %s (\"%s\"). ",
	   GetName(), GetTitle() );
    return fStatus = kInitError;
  }

  return Apparatus::Init(run_time);
}

//_____________________________________________________________________________
Int_t Beam::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "x", "reconstructed x-position at nom. interaction point", "fPosition.fX"},
    { "y", "reconstructed y-position at nom. interaction point", "fPosition.fY"},
    { "z", "reconstructed z-position at nom. interaction point", "fPosition.fZ"},
    { "dir.x", "reconstructed x-component of beam direction", "fDirection.fX"},
    { "dir.y", "reconstructed y-component of beam direction", "fDirection.fY"},
    { "dir.z", "reconstructed z-component of beam direction", "fDirection.fZ"},
    { 0 }
  };

  DefineVarsFromList( vars, mode );

  // Define the variables for the beam info subobject
  DefineVarsFromList( GetRVarDef(), mode );

  return 0;
}

//_____________________________________________________________________________
void Beam::Update()
{
  // Update the fBeamIfo data with the info from the current event

  THaRunParameters* rp = gHaRun->GetParameters();
  if( rp )
    fBeamIfo.Set( rp->GetBeamP(), fDirection, fPosition,
		  rp->GetBeamPol() );
  else
    fBeamIfo.Set( kBig, fDirection, fPosition, kBig );
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

ClassImp(Podd::Beam)


