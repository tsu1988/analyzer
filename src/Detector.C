//*-- Author :    Ole Hansen   15-May-00

//////////////////////////////////////////////////////////////////////////
//
// Detector
//
//////////////////////////////////////////////////////////////////////////

#include "Detector.h"
#include "THaApparatus.h"

namespace Podd {

//_____________________________________________________________________________
Detector::Detector( const char* name, const char* description,
		    THaApparatus* apparatus )
  : DetectorBase(name,description), fApparatus(apparatus)
{
  // Constructor

  if( !name || !*name ) {
    Error( "Detector()", "Must construct detector with valid name! "
	   "Object construction failed." );
    MakeZombie();
    return;
  }
}

//_____________________________________________________________________________
Detector::Detector( ) : fApparatus(0) {
  // for ROOT I/O only
}

//_____________________________________________________________________________
Detector::~Detector()
{
  // Destructor
}

//_____________________________________________________________________________
THaApparatus* Detector::GetApparatus() const
{
  return static_cast<THaApparatus*>(fApparatus.GetObject());
}

//_____________________________________________________________________________
void Detector::SetApparatus( THaApparatus* apparatus )
{
  // Associate this detector with the given apparatus.
  // Only possible before initialization.

  if( IsInit() ) {
    Warning( Here("SetApparatus()"), "Cannot set apparatus. "
	     "Object already initialized.");
    return;
  }
  fApparatus = apparatus;
}

//_____________________________________________________________________________
void Detector::MakePrefix()
{
  // Set up name prefix for global variables. Internal function called
  // during initialization.

  THaApparatus *app = GetApparatus();
  const char* basename = (app != 0) ? app->GetName() : 0;
  DetectorBase::MakePrefix( basename );

}

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

ClassImp(Podd::Detector)
