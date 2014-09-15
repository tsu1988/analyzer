#ifndef Podd_DetectorBase
#define Podd_DetectorBase

//////////////////////////////////////////////////////////////////////////
//
// DetectorBase
//
//////////////////////////////////////////////////////////////////////////

#include "AnalysisObject.h"
#include "TVector3.h"
#include <vector>

class THaDetMap;

namespace Podd {

class DetectorBase : public AnalysisObject {

public:
  virtual ~DetectorBase();

  DetectorBase(); // only for ROOT I/O

  virtual Int_t    Decode( const THaEvData& ) = 0;

  THaDetMap*       GetDetMap() const { return fDetMap; }
  Int_t            GetNelem()  const { return fNelem; }
  const TVector3&  GetOrigin() const { return fOrigin; }
  const Float_t*   GetSize()   const { return fSize; }
  Float_t          GetXSize()  const { return 2.0*fSize[0]; }
  Float_t          GetYSize()  const { return 2.0*fSize[1]; }
  Float_t          GetZSize()  const { return fSize[2]; }

  Bool_t           IsInActiveArea( Double_t x, Double_t y ) const;

  Int_t            FillDetMap( const std::vector<Int_t>& values,
			       UInt_t flags=0,
			       const char* here = "FillDetMap" );
  void             PrintDetMap( Option_t* opt="") const;

protected:

  // Mapping
  THaDetMap*  fDetMap;    // Hardware channel map for this detector

  // Configuration
  Int_t       fNelem;     // Number of detector elements (paddles, mirrors)

  // Geometry
  TVector3    fOrigin;    // Center position of detector (m)
  Float_t     fSize[3];   // Detector size in x,y,z (m) - x,y are half-widths

  virtual Int_t ReadGeometry( FILE* file, const TDatime& date,
			      Bool_t required = kFALSE );

  DetectorBase( const char* name, const char* description );

  ClassDef(DetectorBase,1)   //ABC for a detector or subdetector
};

} // end namespace Podd

// Backwards-compatibility
#define THaDetectorBase Podd::DetectorBase

#endif
