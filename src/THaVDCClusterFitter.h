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
#include <cassert>

namespace VDC {
  struct FitCoord_t {
    FitCoord_t( Double_t _x, Double_t _y, Double_t _w = 1.0, Int_t _s = 1 )
      : x(_x), y(_y), w(_w), s(_s) {}
    Double_t x, y, w;
    Int_t s;
  };
  struct FitRes_t {
    FitRes_t() : n(0), chi2(kBig) { assert( n == 2 || n == 3 ); }
    // Fill lower triangle of covariance matrix
    void fill_lower() {
      if( n == 2 )
	cov[1][0] = cov[0][1];
      else {
	for( int i=0; i<n; ++i )
	  for( int j=i+1; j<n; ++j )
	    cov[j][i] = cov[i][j];
      }
    }
    // Multiply coefficients and covariances with constant (only upper triangle)
    FitRes_t& operator*=( Double_t a ) {
      for( int i=0; i<n; ++i ) {
	coef[i] *= a;
	for( int j=i; j<n; ++j )
	  cov[i][j] *= a;
      }
      return *this;
    }
    Int_t     n;
    Double_t  coef[3];
    Double_t  cov[3][3];
    Double_t  chi2;
    Int_t     ndof;
  };

  typedef std::vector<FitCoord_t> Vcoord_t;
  typedef Vcoord_t::size_type     vcsiz_t;
}
using namespace VDC;

class THaVDCClusterFitter : public THaVDCCluster {
public:
  THaVDCClusterFitter( THaVDCPlane* owner, UInt_t size = kDefaultNHit );
  THaVDCClusterFitter( const Vhit_t& hits, THaVDCPlane* owner );
  virtual ~THaVDCClusterFitter() {}

  enum EMode { kSimple, kLinearT0, kT0 };

  bool           EstTrackParameters();
  void           ConvertTimeToDist( Double_t t0 = 0 );
  void           FitTrack( EMode mode = kT0 );

  // TObject function overrides
  virtual void   Clear( Option_t* opt="" );

  void           SetMaxT0( Double_t t0 ) { fMaxT0 = t0; }
  void           SetWeighted( Bool_t w = true ) { fWeighted = w; }

protected:
  // Workspace for fitting routines
  Vcoord_t         fCoord;          // coordinates to be fit
  Double_t         fPosOffset;      // Position offset subtracted from fCoord[i].x
  Double_t         fMaxT0;          // maximum allowable estimated t0
  Bool_t           fWeighted;       // Do weighted fit of measured drift distances

  Int_t  FitSimpleTrack();
  Int_t  FitNLTrack();        // Non-linear 3-parameter fit
  Int_t  LinearClusterFitWithT0();

  chi2_t CalcChisquare( Double_t slope, Double_t icpt, Double_t d0 ) const;

  Int_t  SetupCoordinates();
  void   AcceptResults( const FitRes_t& res );

  ClassDef(THaVDCClusterFitter,0)     // A VDC cluster with fit methods
};

//////////////////////////////////////////////////////////////////////////////

#endif
