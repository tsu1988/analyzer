#ifndef Podd_SpectrometerDetector
#define Podd_SpectrometerDetector

//////////////////////////////////////////////////////////////////////////
//
// SpectrometerDetector
//
// Abstract base class for a generic spectrometer detector.
//
// This is a specialized detector class that supports the concept of
// "tracking" and "PID" detectors.
//
//////////////////////////////////////////////////////////////////////////

#include "Detector.h"

class THaTrack;

namespace Podd {

class SpectrometerDetector : public Detector {

public:
  virtual ~SpectrometerDetector();

  virtual Bool_t   IsTracking() = 0;
  virtual Bool_t   IsPid()      = 0;

          bool     CheckIntercept( THaTrack* track );
          bool     CalcInterceptCoords( THaTrack* track,
					Double_t& x, Double_t& y );
          bool     CalcPathLen( THaTrack* track, Double_t& t );

  SpectrometerDetector();       // for ROOT I/O only

protected:

  // Geometry data
  TVector3  fXax;                  // X axis of the detector plane
  TVector3  fYax;                  // Y axis of the detector plane
  TVector3  fZax;                  // Normal to the detector plane

  virtual void  DefineAxes( Double_t rotation_angle );

          bool  CalcTrackIntercept( THaTrack* track, Double_t& t,
				    Double_t& ycross, Double_t& xcross);

  //Only derived classes may construct me
  SpectrometerDetector( const char* name, const char* description,
			Apparatus* a = NULL );

  ClassDef(SpectrometerDetector,1)   //ABC for a spectrometer detector
};

} // end namespace Podd

// Backwards-compatibility
#define THaSpectrometerDetector Podd::SpectrometerDetector

#endif
