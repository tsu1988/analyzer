#ifndef Podd_TrackingDetector
#define Podd_TrackingDetector

//////////////////////////////////////////////////////////////////////////
//
// TrackingDetector.h
//
//////////////////////////////////////////////////////////////////////////

#include "SpectrometerDetector.h"

class TClonesArray;
class THaTrack;
class THaTrackID;

namespace Podd {

class TrackingDetector : public SpectrometerDetector {

public:
  virtual ~TrackingDetector();

  virtual Int_t    CoarseTrack( TClonesArray& tracks ) = 0;
  virtual Int_t    FineTrack( TClonesArray& tracks )  = 0;
  // For backward-compatibility
  virtual Int_t    FindVertices( TClonesArray& /* tracks */ ) { return 0; }
          Bool_t   IsTracking() { return kTRUE; }
          Bool_t   IsPid()      { return kFALSE; }

	  TrackingDetector();   // for ROOT I/O only
protected:

  virtual THaTrack* AddTrack( TClonesArray& tracks,
			      Double_t x, Double_t y,
			      Double_t theta, Double_t phi,
			      THaTrackID* ID = NULL );

  //Only derived classes may construct me

  TrackingDetector( const char* name, const char* description,
		    Apparatus* a = NULL );

  ClassDef(TrackingDetector,1)   //ABC for a generic tracking detector
};

} // end namespace Podd

// Backwards-compatibility
#define THaTrackingDetector Podd::TrackingDetector

#endif
