#ifndef ROOT_THaNonTrackingDetector
#define ROOT_THaNonTrackingDetector

//////////////////////////////////////////////////////////////////////////
//
// THaNonTrackingDetector.h
//
// Abstract base class for a generic non-tracking spectrometer detector. 
//
// This is a special THaSpectrometerDetector -- any detector that
// is not a tracking detector.  This includes PID detectors.
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"

class TClonesArray;

class THaNonTrackingDetector : public THaSpectrometerDetector {
  
public:
  virtual ~THaNonTrackingDetector();
  
  virtual Int_t    CoarseProcess( TClonesArray& tracks ) = 0;
  virtual Int_t    FineProcess( TClonesArray& tracks )  = 0;
          Bool_t   IsTracking() { return kFALSE; }
  virtual Bool_t   IsPid()      { return kFALSE; }

  virtual void Draw(TGeometry* geom,const THaEvData& evdata,const Option_t* opt =NULL) {};
  //  virtual void Draw(TGeometry* geom,const Option_t* opt = NULL) {};
  virtual void Draw(const Option_t* opt = NULL){};

protected:

  //Only derived classes may construct me

  THaNonTrackingDetector( const char* name, const char* description,
			  THaApparatus* a = NULL);

  ClassDef(THaNonTrackingDetector,0)  //ABC for a non-tracking spectrometer detector
};

#endif
