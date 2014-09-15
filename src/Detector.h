#ifndef Podd_Detector
#define Podd_Detector

//////////////////////////////////////////////////////////////////////////
//
// Detector
//
// Abstract base class for a generic detector. This class describes an
// actual detector (not subdetector) and can be added to an apparatus.
//
//////////////////////////////////////////////////////////////////////////

#include "DetectorBase.h"
#include "TRef.h"

namespace Podd {

class Apparatus;

class Detector : public DetectorBase {

public:
  virtual ~Detector();
  Apparatus*     GetApparatus() const;
  virtual void   SetApparatus( Apparatus* );

  Detector();  // for ROOT I/O only

protected:

  virtual void MakePrefix();

  //Only derived classes may construct me
  Detector( const char* name, const char* description,
	    Apparatus* apparatus = 0 );

private:
  TRef  fApparatus;         // Apparatus containing this detector

  ClassDef(Detector,1)      // Abstract base class for a detector
};

} // end namespace Podd

// Backwards-compatibility
#define THaDetector Podd::Detector
#ifndef THaApparatus
# define THaApparatus Podd::Apparatus
#endif

#endif
