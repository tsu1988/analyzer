#ifndef Podd_THaBeam
#define Podd_THaBeam

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
//////////////////////////////////////////////////////////////////////////

#include "Apparatus.h"
#include "THaBeamModule.h"
#include "THaRunParameters.h"
#include "TVector3.h"
#include "VarDef.h"

namespace Podd {

class Beam : public Apparatus, public THaBeamModule {

public:
  virtual ~Beam();

  virtual EStatus Init( const TDatime& run_time );

  virtual const TVector3& GetPosition()  const { return fPosition; }
  virtual const TVector3& GetDirection() const { return fDirection; }
  THaRunParameters*   GetRunParameters() const { return fRunParam; }

protected:

  virtual Int_t  DefineVariables( EMode mode = kDefine );
          void   Update();

  TVector3  fPosition;   // Beam position at the target (usually z=0) (meters)
  TVector3  fDirection;  // Beam direction vector (arbitrary units)

  THaRunParameters* fRunParam; // Pointer to parameters of current run

  Beam( const char* name, const char* description ) ;

  ClassDef(Beam,1)    // ABC for an apparatus providing beam information
};

} // end namespace Podd

// Backwards-compatibility
#define THaBeam Podd::Beam

#endif

