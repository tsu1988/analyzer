#ifndef ROOT_THaDetector
#define ROOT_THaDetector

//////////////////////////////////////////////////////////////////////////
//
// THaDetector
//
// Abstract base class for a generic Hall A detector. This class 
// describes an actual detector (not subdetector) and can be added to
// an apparatus.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"

class THaApparatus;
class THaTrack;
class TGeometry;

class THaDetector : public THaDetectorBase {
  
public:
  virtual ~THaDetector();
  
  THaApparatus*  GetApparatus() const   { return fApparatus; }
  virtual void   SetApparatus( THaApparatus* );

  virtual void Draw(TGeometry* geom, const Option_t* opt=NULL);
  virtual void Draw(TGeometry* geom, THaTrack* track, Double_t& t, const Option_t* opt=NULL);
  virtual void Draw(const Option_t* opt=NULL);
  virtual void Draw(TGeometry* geom, const THaEvData& evdata, const Option_t* opt = NULL);

protected:
  THaApparatus*  fApparatus;        // Apparatus containing this detector

//Only derived classes may construct me

  THaDetector( const char* name, const char* description, 
	       THaApparatus* apparatus = NULL );  

  virtual void MakePrefix();

  ClassDef(THaDetector,0)   //Abstract base class for a Hall A detector
};

#endif
