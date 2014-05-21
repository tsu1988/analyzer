#ifndef ROOT_THaVDCClusterFitter
#define ROOT_THaVDCClusterFitter

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCClusterFitter                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCCluster.h"
#include <utility>
#include <vector>

namespace VDC {
  struct FitCoord_t {
    FitCoord_t( Double_t _x, Double_t _y, Double_t _w = 1.0, Int_t _s = 1 )
      : x(_x), y(_y), w(_w), s(_s) {}
    Double_t x, y, w;
    Int_t s;
  };
  typedef std::vector<FitCoord_t> Vcoord_t;
}
using namespace VDC;

class THaVDCClusterFitter : public THaVDCCluster {
public:
  THaVDCClusterFitter( THaVDCPlane* owner );
  THaVDCClusterFitter( const Vhit_t& hits, THaVDCPlane* owner );
  virtual ~THaVDCClusterFitter() {}

  enum EMode { kSimple, kWeighted, kLinearT0, kT0 };

  void           EstTrackParameters();
  void           ConvertTimeToDist( Double_t t0 = 0 );
  void           FitTrack( EMode mode = kT0 );

  // TObject function overrides
  virtual void   Clear( Option_t* opt="" );

protected:
  // Workspace for fitting routines
  Vcoord_t       fCoord;             // coordinates to be fit

  void   FitSimpleTrack( Bool_t weighted = false );
  void   FitNLTrack();        // Non-linear 3-parameter fit
  void   Linear3DFit( Double_t& slope, Double_t& icpt, Double_t& d0 ) const;
  Int_t  LinearClusterFitWithT0();

  chi2_t CalcChisquare( Double_t slope, Double_t icpt, Double_t d0 ) const;

  ClassDef(THaVDCClusterFitter,0)     // A VDC cluster with fit methods
};

//////////////////////////////////////////////////////////////////////////////

#endif
