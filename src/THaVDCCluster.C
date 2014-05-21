///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
// A group of VDC hits and routines for linear and non-linear fitting of     //
// drift distances.                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "THaVDCPlane.h"
#include "THaTrack.h"
#include "TMath.h"
#include "TClass.h"

#include <iostream>
#include <cassert>

using namespace std;

const Double_t VDC::kBig = 1e38;  // Arbitrary large value
const Vhit_t::size_type kDefaultNHit = 16;

#define ALL(c) (c).begin(), (c).end()

//_____________________________________________________________________________
THaVDCCluster::THaVDCCluster( THaVDCPlane* owner )
  : fPlane(owner), fPointPair(0), fTrack(0), fTrkNum(0),
    fSlope(kBig), fLocalSlope(kBig), fSigmaSlope(kBig),
    fInt(kBig), fSigmaInt(kBig), fT0(kBig), fSigmaT0(kBig),
    fPivot(0), fTimeCorrection(0),
    fFitOK(false), fChi2(kBig), fNDoF(0.0), fClsBeg(kMaxInt-1), fClsEnd(-1)
{
  // Constructor

  fHits.reserve(kDefaultNHit);
}

//_____________________________________________________________________________
THaVDCCluster::THaVDCCluster( const Vhit_t& hits, THaVDCPlane* owner )
  : fHits(hits), fPlane(owner), fPointPair(0), fTrack(0), fTrkNum(0),
    fSlope(kBig), fLocalSlope(kBig), fSigmaSlope(kBig),
    fInt(kBig), fSigmaInt(kBig), fT0(kBig), fSigmaT0(kBig),
    fPivot(0), fTimeCorrection(0), fFitOK(false), fChi2(kBig), fNDoF(0.0)
{
  // Constructor

  SetBegEnd();
}

//_____________________________________________________________________________
void THaVDCCluster::AddHit( THaVDCHit* hit )
{
  // Add a hit to the cluster

  assert( hit );
  fHits.push_back( hit );

  if( fClsBeg > hit->GetWireNum() )
    fClsBeg = hit->GetWireNum();
  if( fClsEnd < hit->GetWireNum() )
    fClsEnd = hit->GetWireNum();
}

//_____________________________________________________________________________
void THaVDCCluster::SetHits( const Vhit_t& hits )
{
  // Assign all hits in the given vector to the cluster

  fHits.assign( ALL(hits) );
  SetBegEnd();
}

//_____________________________________________________________________________
void THaVDCCluster::SwapHits( Vhit_t& hits )
{
  // Swap all hits in the given vector with the ones currently in the cluster.
  // This is more efficient than SetHits, but invalidates the input vector.

  fHits.swap( hits );
  SetBegEnd();
}

//_____________________________________________________________________________
void THaVDCCluster::SetBegEnd()
{
  // Set wire number range of this cluster

  Int_t nhits = GetSize();
  if( nhits == 0 ) {
    fClsBeg = kMaxInt-1;
    fClsEnd = -1;
  } else {
    // We don't assume the hits are sorted
    for( Int_t i = 0; i < nhits; ++i ) {
      THaVDCHit* hit = fHits[i];
      assert( hit );
      if( fClsBeg > hit->GetWireNum() )
	fClsBeg = hit->GetWireNum();
      if( fClsEnd < hit->GetWireNum() )
	fClsEnd = hit->GetWireNum();
    }
  }
}

//_____________________________________________________________________________
void THaVDCCluster::Clear( const Option_t* )
{
  // Clear the contents of the cluster and reset status

  fHits.clear();
  fPivot   = 0;
  fPlane   = 0;
  fPointPair = 0;
  fTrack   = 0;
  fTrkNum  = 0;
  fClsBeg  = -1;
  fClsEnd  = kMaxInt;
}

//_____________________________________________________________________________
void THaVDCCluster::ClearFit()
{
  // Clear fit results only

  fSlope      = kBig;
  fLocalSlope = kBig;
  fSigmaSlope = kBig;
  fInt        = kBig;
  fSigmaInt   = kBig;
  fT0         = 0.0;
  fSigmaT0    = kBig;
  fFitOK      = false;
  fLocalSlope = kBig;
  fChi2       = kBig;
  fNDoF       = 0.0;
}

//_____________________________________________________________________________
Int_t THaVDCCluster::Compare( const TObject* obj ) const
{
  // Compare this cluster to another via the wire number of the pivot.
  // Returns -1 if comparison cannot be made (unlike class, no pivot).

  if( !obj || IsA() != obj->IsA() )
    return -1;

  const THaVDCCluster* rhs = static_cast<const THaVDCCluster*>( obj );
  if( GetPivotWireNum() < rhs->GetPivotWireNum() )
    return -1;
  if( GetPivotWireNum() > rhs->GetPivotWireNum() )
    return +1;

  return 0;
}

//_____________________________________________________________________________
chi2_t THaVDCCluster::CalcDist()
{
  // Calculate and store the distance of the global track to the wires.
  // We can then inspect the quality of the fits.
  //
  // This is actually a bit tedious: we need to project the track
  // (given by its detector coordinates) onto this cluster's plane (u or v),
  // then find the track's z coordinate for each active wire's position.
  // It takes two pages of vector algebra.
  //
  // Return value: sum of squares of distances and number of hits used

  assert( fTrack );

  Int_t npt  = 0;
  Double_t chi2 = 0;
  for (int j = 0; j < GetSize(); j++) {
    // Use only hits marked to belong to this track
    if( fHits[j]->GetTrkNum() != fTrack->GetTrkNum() )
      continue;
    Double_t u     = fHits[j]->GetPos();    // u (v) coordinate of wire
    Double_t sina  = fPlane->GetSinAngle();
    Double_t cosa  = fPlane->GetCosAngle();
    Double_t denom = cosa*fTrack->GetDTheta() + sina*fTrack->GetDPhi();
    Double_t z     = (u - cosa*fTrack->GetDX() - sina*fTrack->GetDY()) / denom;
    Double_t dz    = z - fPlane->GetZ();
    fHits[j]->SetFitDist(TMath::Abs(dz));
    chi2 += dz*dz;
    npt++;
  }
  return make_pair( chi2, npt );
}

//_____________________________________________________________________________
void THaVDCCluster::CalcLocalDist()
{
  //FIXME: clean this up - duplicate of chi2 calculation
  // Calculate and store the distance of the local fitted track to the wires.
  // We can then inspect the quality of the local fits
  for (int j = 0; j < GetSize(); j++) {
    Double_t y = fHits[j]->GetPos();
    if (fLocalSlope != 0. && fLocalSlope < kBig) {
      Double_t X = (y-fInt)/fLocalSlope;
      fHits[j]->SetLocalFitDist(TMath::Abs(X));
    } else {
      fHits[j]->SetLocalFitDist(kBig);
    }
  }
}

//_____________________________________________________________________________
Int_t THaVDCCluster::GetTrackIndex() const
{
  // Return index of assigned track (-1 = none, 0 = first/best, etc.)

  if( !fTrack ) return -1;
  return fTrack->GetIndex();
}

Int_t THaVDCCluster::GetPivotWireNum() const
{
  // Get wire number of cluster pivot (hit with smallest drift distance)

  return fPivot ? fPivot->GetWireNum() : -1;
}

//_____________________________________________________________________________
void THaVDCCluster::SetTrack( THaTrack* track )
{
  // Mark this cluster as used by the given track whose number (index+1) is num

  Int_t num = track ? track->GetTrkNum() : 0;

  // Mark all hits
  // FIXME: naahh - only the hits used in the fit! (ignore for now)
  for( int i=0; i<GetSize(); i++ ) {
    // Bugcheck: hits must either be unused or been used by this cluster's
    // track (so SetTrack(0) can release a cluster from a track)
    assert( fHits[i] );
    assert( fHits[i]->GetTrkNum() == 0 || fHits[i]->GetTrkNum() == fTrkNum );

    fHits[i]->SetTrkNum( num );
  }
  fTrack = track;
  fTrkNum = num;
}

//_____________________________________________________________________________
void THaVDCCluster::CalcChisquare(Double_t& chi2, Int_t& nhits ) const
{
  // given the parameters of the track (slope and intercept), calculate
  // chi2 for the cluster

  Int_t pivotNum = 0;
  for (int j = 0; j < GetSize(); j++) {
    if (fHits[j] == fPivot) {
      pivotNum = j;
    }
  }

  for (int j = 0; j < GetSize(); j++) {
    Double_t x = fHits[j]->GetDist() + fTimeCorrection;
    if (j>pivotNum) x = -x;

    Double_t y = fHits[j]->GetPos();
    Double_t dx = fHits[j]->GetdDist();
    Double_t Y = fSlope * x + fInt;
    Double_t dY = fSlope * dx;
    if (dx <= 0) continue;

    if ( fHits[j] == fPivot ) {
      // test the other side of the pivot wire, take the 'best' choice
      Double_t ox = -x;
      Double_t oY =  fSlope * ox + fInt;
      if ( TMath::Abs(y-Y) > TMath::Abs(y-oY) ) {
	x = ox;
	Y = oY;
      }
    }
    chi2 += (Y - y)/dY * (Y - y)/dY;
    nhits++;
  }
}


//_____________________________________________________________________________
void THaVDCCluster::Print( Option_t* ) const
{
  // Print contents of cluster

  if( fPlane )
    cout << "Plane: " << fPlane->GetPrefix() << endl;
  cout << "Size: " << GetSize() << endl;

  cout << "Wire numbers:";
  for( int i = 0; i < GetSize(); i++ ) {
    cout << " " << fHits[i]->GetWireNum();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire raw times:";
  for( int i = 0; i < GetSize(); i++ ) {
    cout << " " << fHits[i]->GetRawTime();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire times:";
  for( int i = 0; i < GetSize(); i++ ) {
    cout << " " << fHits[i]->GetTime();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire positions:";
  for( int i = 0; i < GetSize(); i++ ) {
    cout << " " << fHits[i]->GetPos();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;
  cout << "Wire drifts:";
  for( int i = 0; i < GetSize(); i++ ) {
    cout << " " << fHits[i]->GetDist();
    if( fHits[i] == fPivot )
      cout << "*";
  }
  cout << endl;

  cout << "intercept(err), local slope(err), global slope, t0(err): "
       << fInt   << " (" << fSigmaInt   << "), "
       << fLocalSlope << " (" << fSigmaSlope << "), "
       << fSlope << ", "
       << fT0    << " (" << fSigmaT0 << "), fit ok: " << fFitOK
       << endl;

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCCluster)
