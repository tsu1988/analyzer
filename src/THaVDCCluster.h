#ifndef ROOT_THaVDCCluster
#define ROOT_THaVDCCluster

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include <utility>
#include <vector>
#include <cassert>

class THaVDCHit;
class THaVDCPlane;
class THaVDCPointPair;
class THaTrack;

namespace VDC {
  typedef std::pair<Double_t,Int_t>  chi2_t;
  typedef THaVDCPointPair VDCpp_t;
  typedef std::vector<THaVDCHit*> Vhit_t;
  typedef Vhit_t::iterator hit_iter_t;

  extern const Double_t kBig;

  inline chi2_t operator+( chi2_t a, const chi2_t& b ) {
    a.first  += b.first;
    a.second += b.second;
    return a;
  };
}
using namespace VDC;

class THaVDCCluster : public TObject {

protected:
  // Stupid rootcint needs to see this first
  static const Vhit_t::size_type kDefaultNHit = 16;

public:
  THaVDCCluster( THaVDCPlane* owner, UInt_t size = kDefaultNHit );
  THaVDCCluster( const Vhit_t& hits, THaVDCPlane* owner );
  THaVDCCluster( const THaVDCCluster& rhs );
  THaVDCCluster& operator=( const THaVDCCluster& rhs );
  virtual ~THaVDCCluster();

  void           AddHit( THaVDCHit* hit );
  void           SetHits( const Vhit_t& hits );
  void           SwapHits( Vhit_t& hits );
  void           ClearFit();
  void           CalcChisquare(Double_t& chi2, Int_t& nhits) const;
  chi2_t         CalcDist();    // calculate global track to wire distances

  // TObject function overrides
  virtual void   Clear( Option_t* opt="" );
  virtual Int_t  Compare( const TObject* obj ) const;
  virtual Bool_t IsSortable() const        { return kTRUE; }
  virtual void   Print( Option_t* opt="" ) const;

  //Get and Set Functions
  THaVDCHit*     GetHit(Int_t i)     const { return fHits[i]; }
  Vhit_t&        GetHits()                 { return fHits; }
  const Vhit_t&  GetHits()           const { return fHits; }
  THaVDCPlane*   GetPlane()          const { return fPlane; }
  Int_t          GetSize ()          const { return fHits.size(); }
  Int_t          GetSpan()           const { return fClsEnd-fClsBeg; }
  Double_t       GetSlope()          const { return fSlope; }
  Double_t       GetLocalSlope()     const { return fLocalSlope; }
  Double_t       GetSigmaSlope()     const { return fSigmaSlope; }
  Double_t       GetIntercept()      const { return fInt; }
  Double_t       GetSigmaIntercept() const { return fSigmaInt; }
  THaVDCHit*     GetPivot()          const;
  Int_t          GetPivotWireNum()   const;
  Double_t       GetTimeCorrection() const { return fTimeCorrection; }
  Double_t       GetT0()             const { return fT0; }
  Double_t       GetSigmaT0()        const { return fSigmaT0; }
  VDCpp_t*       GetPointPair()      const { return fPointPair; }
  THaTrack*      GetTrack()          const { return fTrack; }
  Int_t          GetTrackIndex()     const;
  Int_t          GetTrkNum()         const { return fTrkNum; }
  Double_t       GetChi2()           const { return fChi2; }
  Double_t       GetNDoF()           const { return fNDoF; }
  Bool_t         IsFitOK()           const { return fFitOK; }
  Bool_t         IsUsed()            const { return (fTrack != 0); }

  void           SetBegEnd();
  void           SetSlope( Double_t slope)          { fSlope = slope;}
  void           SetTimeCorrection( Double_t dt )   { fTimeCorrection = dt; }
  void           SetPointPair( VDCpp_t* pp )        { fPointPair = pp; }
  void           SetTrack( THaTrack* track );

  // Hit association
  void           ReleaseHits() const;
  void           ClaimHits()   const;
  Bool_t         HasSharedHits( Bool_t force_check = false ) const;

protected:
  THaVDCPlane*   fPlane;             // Plane the cluster belongs to
  Vhit_t         fHits;              // Hits associated w/this cluster
  Int_t          fPivotIdx;          // Index of hit with smallest drift time
  Int_t          fClsBeg; 	     // Starting wire number
  Int_t          fClsEnd;            // Ending wire number
  VDCpp_t*       fPointPair;         // Lower/upper combo we're assigned to
  THaTrack*      fTrack;             // Track the cluster belongs to
  Int_t          fTrkNum;            // Number of the track using this cluster

  // Fit results
  Double_t       fSlope;             // Current best estimate of actual slope
  Double_t       fLocalSlope;        // Fitted slope, from FitTrack()
  Double_t       fSigmaSlope;        // Error estimate of fLocalSlope from fit
  Double_t       fInt, fSigmaInt;    // Intercept and error estimate
  Double_t       fT0, fSigmaT0;      // Fitted common timing offset and error
  Double_t       fTimeCorrection;    // Time offset applied to drift times
  Double_t       fChi2;              // chi2 for the cluster (using fSlope)
  Double_t       fNDoF;              // NDoF in local chi2 calculation
  Bool_t         fFitOK;             // Flag indicating that fit results valid

  enum EBool3 { kUndefined, kFalse, kTrue };
  mutable EBool3 fSharedHits;        // Cache for HasSharedHits
  mutable Bool_t fHitsClaimed;       // If true, ClaimHits has run

  void   CalcLocalDist();     // calculate the local track to wire distances

  ClassDef(THaVDCCluster,0)          // A group of VDC hits
};

//_____________________________________________________________________________
inline THaVDCHit* THaVDCCluster::GetPivot() const
{
  // Get hit with smallest drift distance

  assert( (!fHits.empty() || fPivotIdx == -1) && fPivotIdx < GetSize() );
  return ( fPivotIdx < 0 ) ? 0 : fHits[fPivotIdx];
}

//////////////////////////////////////////////////////////////////////////////

#endif
