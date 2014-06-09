///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
// A group of VDC hits and routines for linear and non-linear fitting of     //
// drift distances.                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCClusterFitter.h"
#include "THaVDCHit.h"
#include "THaVDCPlane.h"
#include "TMath.h"

// ROOT matrix magic for non-linear fit routine
#include "TMatrixD.h"
#include "TVectorD.h"
#include "TDecompQRH.h"
#include "TDecompSVD.h"
#include "TDecompBase.h"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <iterator>
#include <numeric>

using namespace std;

#define ALL(c) (c).begin(), (c).end()
#define RALL(c) (c).rbegin(), (c).rend()

//_____________________________________________________________________________
THaVDCClusterFitter::THaVDCClusterFitter( THaVDCPlane* owner, UInt_t size )
  : THaVDCCluster(owner,size)
{
  // Constructor

  fCoord.reserve(size);
}

//_____________________________________________________________________________
THaVDCClusterFitter::THaVDCClusterFitter( const Vhit_t& hits,
					  THaVDCPlane* owner )
  : THaVDCCluster(hits,owner)
{
  // Constructor

  fCoord.reserve(fHits.size());
}

//_____________________________________________________________________________
void THaVDCClusterFitter::Clear( const Option_t* opt )
{
  // Clear the contents of the cluster and reset status

  THaVDCCluster::Clear(opt);
  fCoord.clear();
}

//_____________________________________________________________________________
bool THaVDCClusterFitter::EstTrackParameters()
{
  // Estimate Track Parameters
  // Calculates pivot wire and uses its position as position of cluster
  // Estimates the slope based on the distance between the first and last
  // hits in the U (or V) and detector Z directions

  assert( fPlane );

  fFitOK = false;
  if( GetSize() == 0 )
    return false;

  // Find pivot
  fPivotIter = min_element( ALL(fHits), THaVDCHit::ByTime() );
  assert( fPivotIter != fHits.end() );  // size > 0 implies at least one hit
  fPivot = *fPivotIter;
  assert( fPivot );

  // For good clusters, pivot must not be at beginning or end of region
  if( fPivot == fHits.front() || fPivot == fHits.back() )
    return false;

  // Rough estimate of t0. Ensure that pivot time is always positive
  const Double_t minSlope = 0.9;  // FIXME: better estimate
  const Double_t vdrift = fPlane->GetDriftVel();  // m/s
  const Double_t maxPivotTime = 0.5*fPlane->GetWSpac() / (vdrift * minSlope);
  assert( maxPivotTime > 0 );
  Double_t pivotTime = fPivot->GetTime();
  Double_t timeOffset = 0.0;      // offset to subtract from all drift times
  if( pivotTime < 0.0 || pivotTime > maxPivotTime ) {
    timeOffset = pivotTime - 0.5*maxPivotTime;
  }
  if( TMath::Abs(timeOffset) > fMaxT0 )
    return false;

  // Find the approximate slope
  if( GetSize() > 1 ) {
    THaVDCHit *first_hit = fHits.front(), *last_hit = fHits.back();
    // Assume that the drift time changes sign between first and last wire
    Double_t dt = last_hit->GetTime() + first_hit->GetTime() - 2.*timeOffset;
    Double_t dx = last_hit->GetPos()  - first_hit->GetPos();
    assert( dt > 0 );   // else error in calculating timeOffset
    assert( dx > 0 );   // else hit ordering error
    fSlope = dx / (vdrift*dt);
    // Protect time-to-distance converter from crazy input
    // FIXME: but maybe we should just fail here already?
    // if( fSlope < 0.25 || fSlope > 4.0 )
    //   fSlope = 1.0;
  } else
    fSlope = 1.0;

  // Current best estimates for intercept and t0
  fInt   = fPivot->GetPos();
  fT0    = timeOffset;
  fFitOK = true;

  return true;
}

//_____________________________________________________________________________
void THaVDCClusterFitter::ConvertTimeToDist( Double_t t0 )
{
  // Convert TDC Times in wires to drift distances

  //Do conversion for each hit in cluster
  for( Int_t i = 0; i < GetSize(); i++ )
    fHits[i]->ConvertTimeToDist(fSlope,t0-fTimeCorrection);
}

//_____________________________________________________________________________
inline static Double_t PosSum( Double_t val, const THaVDCHit* hit )
{
  return val + hit->GetPos();
}

//_____________________________________________________________________________
class FillCoord
{
public:
  FillCoord( Vcoord_t& _coord, Vcoord_t::size_type _pivot, Double_t _off,
	     bool _weighted = true )
    : m_coord(_coord), m_pivot(_pivot), m_offset(_off), m_weighted(_weighted)
  { m_coord.clear(); }
  void operator() ( const THaVDCHit* hit )
  {
    // In order to take into account the varying uncertainty in the
    // drift distance, we will be working with the X' and Y', and
    //       Y' = F + G X'
    //  where Y' = X, and X' = Y  (swapping it around)
    Double_t x = hit->GetPos();
    Double_t y = hit->GetDist();
    Double_t wt;
    if( m_weighted ) {
      wt = hit->GetdDist();
      if (wt>0)   // the hit will be ignored if the uncertainty is <= 0
	wt = 1./(wt*wt);
      else
	wt = -1.;
    } else {
      wt = 1.0;
    }
    Int_t s = (m_coord.size() < m_pivot) ? -1 : 1;
    m_coord.push_back( FitCoord_t(x-m_offset,y,wt,s) );

    assert( m_coord.size() == 1 ||
	    m_coord[m_coord.size()-2].x < m_coord.back().x );
  }
private:
  Vcoord_t&           m_coord;
  Vcoord_t::size_type m_pivot;
  Double_t            m_offset;
  bool                m_weighted;
};

//_____________________________________________________________________________
Int_t THaVDCClusterFitter::SetupCoordinates()
{
  // Calculate fPivotIdx and fPosOffset, and fill fCoords with offset
  // coordinates suitable for fitting.
  // Must be called after ConvertTimeToDistance.

  if( fHits.empty() )
    return -1;

  // Determine index of pivot wire
  fPivotIdx = distance(fHits.begin(),fPivotIter);
  assert( fPivotIdx >= 0 && fPivotIdx < GetSize() );

  // Align fit coordinates such that we end up with sum x_i = 0.
  // This improves the numerical accuracy of the linear fits and eliminates
  // certain correlations between the fitted parameters
  fPosOffset = accumulate( ALL(fHits), 0.0, PosSum ) / fHits.size();

  // Copy hits to fCoord, applying position and time corrections
  for_each( ALL(fHits), FillCoord(fCoord,fPivotIdx,fPosOffset,fWeighted) );

  assert( fCoord.size() == fHits.size() );
  assert( fCoord[fPivotIdx].s == 1 );

  return 0;
}

//_____________________________________________________________________________
void THaVDCClusterFitter::FitTrack( EMode mode )
{
  // Fit track to drift distances. Supports three modes:
  //
  // kSimple:    Linear fit, ignore t0 and multihits
  // kWeighted:  Linear fit with weights, ignore t0
  // kT0:        Fit t0, but ignore mulithits

  switch( mode ) {
  case kSimple:
    FitSimpleTrack(false);
    break;
  case kWeighted:
    FitSimpleTrack(true);
    break;
  case kLinearT0:
    LinearClusterFitWithT0();
    break;
  case kT0:
    FitNLTrack();
    break;
  }
  CalcLocalDist();
}

//_____________________________________________________________________________
struct CalcParamSums
{
  CalcParamSums() : W(0), X(0), XX(0), Y(0), XY(0), YY(0), S(0), SX(0), SY(0),
		    SYX(0) /*, WW(0), XW(0), XXW(0) */ {}
  void operator() ( const FitCoord_t& coord )
  {
    if( coord.w <= 0 ) return; // ignore coordinates with negative weight
    Double_t x = coord.x, y = coord.y, w = coord.w, s = coord.s /*,ww = w*w */;
    W   += w;
    X   += x * w;
    XX  += x * x * w;
    Y   += y * w;
    XY  += x * y * w;
    YY  += y * y * w;
    S   += s * w;
    SX  += s * x * w;
    SY  += s * y * w;
    SYX += s * y * x * w;
    // WW  += ww;
    // XW  += x * ww;
    // XXW += x * x * ww;
  }
  Double_t W, X, XX, Y, XY, YY, S, SX, SY, SYX /*, WW, XW, XXW */;
};

//_____________________________________________________________________________
class CalcChi2
{
public:
  CalcChi2( const FitRes_t& _res ) : m_res(_res), m_chi2(0), m_npt(0) {}
  void operator() ( const FitCoord_t& coord )
  {
    if( coord.w <= 0 ) return; // ignore coordinates with negative weight
    Double_t x  = coord.x, y = coord.y*coord.s, w = coord.w;
    Double_t yp = x*m_res.coef[0] + m_res.coef[1];
    if( m_res.n == 3 )  yp += coord.s*m_res.coef[2];
    Double_t d  = y-yp;
    m_chi2     += d*d*w;
    m_npt++;
  }
  chi2_t   GetResult() const { return make_pair(m_chi2, m_npt); }
  Double_t GetChi2()   const { return m_chi2; }
  Int_t    GetNpt()    const { return m_npt; }
private:
  const FitRes_t& m_res;
  Double_t m_chi2;
  Int_t    m_npt;
};

//_____________________________________________________________________________
static bool Linear2ParamFit( const Vcoord_t& coord, FitRes_t& res )
{
  // Linear 2-parameter fit (slope & intercept)

  CalcParamSums sum = for_each( ALL(coord), CalcParamSums() );

  // Standard expressions for linear regression s_i * y_i = a * x_i + b
  // (see e.g. Bevington 3rd ed. p. 105 and p. 109)
  Double_t Delta = sum.XX * sum.W - sum.X * sum.X;
  if( Delta == 0.0 ) return false;

  res.n = 2;
  res.coef[0]   = ( sum.SYX * sum.W  - sum.SY * sum.X   ); // slope
  res.coef[1]   = ( sum.XX  * sum.SY - sum.X  * sum.SYX ); // icpt
  res.cov[0][0] =  sum.W;   // sigma_a^2
  res.cov[1][1] =  sum.XX;  // sigma_b^2
  res.cov[0][1] = -sum.X;   // sigma_ab covariance
  res *= 1.0/Delta;
  res.fill_lower();

  // calculate chi2 for the track given the fitted slope and intercept
  chi2_t chi2 = for_each( ALL(coord), CalcChi2(res) ).GetResult();
  res.chi2 = chi2.first;
  res.ndof = chi2.second - 2;

  return true;
}

//_____________________________________________________________________________
static bool Linear3ParamFit( const Vcoord_t& coord, FitRes_t& res )
{
  // Linear 3-parameter fit (slope, intercept, distance offset)

  CalcParamSums sum = for_each( ALL(coord), CalcParamSums() );

  Double_t Delta =
    sum.XX  * ( sum.W   * sum.W - sum.S  * sum.S   ) -
    sum.X   * ( sum.X   * sum.W - sum.S  * sum.SX  ) +
    sum.SX  * ( sum.X   * sum.S - sum.W  * sum.SX  );

  if( Delta == 0.0 ) return false;

  res.n = 3;
  res.coef[0] =  // slope
    sum.SYX * ( sum.W   * sum.W - sum.S  * sum.S   ) -
    sum.SY  * ( sum.X   * sum.W - sum.S  * sum.SX  ) +
    sum.Y   * ( sum.X   * sum.S - sum.W  * sum.SX  );

  res.coef[1] =  // intercept
    sum.XX  * ( sum.SY  * sum.W - sum.Y  * sum.S   ) -
    sum.X   * ( sum.SYX * sum.W - sum.Y  * sum.SX  ) +
    sum.SX  * ( sum.SYX * sum.S - sum.SY * sum.SX  );

  res.coef[2] =  // distance offset
    sum.XX  * ( sum.W   * sum.Y  - sum.S  * sum.SY  ) -
    sum.X   * ( sum.X   * sum.Y  - sum.S  * sum.SYX ) +
    sum.SX  * ( sum.X   * sum.SY - sum.W  * sum.SYX );


  //FIXME: calculate covariance matrix

  res *= 1.0/Delta;
  res.fill_lower();

  // calculate chi2 for the track given the fitted slope and intercept
  chi2_t chi2 = for_each( ALL(coord), CalcChi2(res) ).GetResult();
  res.chi2 = chi2.first;
  res.ndof = chi2.second - 3;

  return true;
}

//_____________________________________________________________________________
Int_t THaVDCClusterFitter::FitSimpleTrack( Bool_t weighted )
{
  // Perform linear fit on drift times. Calculates slope, intercept, and
  // errors. If weighted = true, uses the estimated drift distance uncertain-
  // ties for weighting hits. Assumes t0 = 0.

  // For this function, the final results is (m,b) in  Y = m X + b
  //   X = Drift Distance
  //   Y = Position of Wires

  Int_t err = SetupCoordinates();
  if( err )
    return err;

  FitRes_t res1, res2;
  Linear2ParamFit( fCoord, res1 );
  fCoord[fPivotIdx].s = -1;    // Flip sign of pivot drift distance
  Linear2ParamFit( fCoord, res2 );

  bool res1_is_best = ( res1.chi2 < res2.chi2 );
  const FitRes_t& best = ( res1_is_best ) ? res1 : res2;
  if( res1_is_best )
    fCoord[fPivotIdx].s = 1;  // Restore pivot wire sign used for this fit

  if( best.chi2 >= kBig )
    return -1;

  AcceptResults( best );
  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCClusterFitter::LinearClusterFitWithT0()
{
  // Perform linear fit on drift times. Calculates slope, intercept, time
  // offset t0, and errors.
  // Accounts for different uncertainties of each drift distance.
  //
  // Fits the parameters m, b, and d0 in
  //
  // s_i ( d_i + d0 ) = m x_i + b
  //
  // where
  //  s_i   = sign of ith drift distance (+/- 1)
  //  d_i   = ith measured drift distance
  //  x_i   = ith wire position
  //
  // The sign vector, s_i, is automatically optimized to yield the smallest
  // chi^2. Normally, s_i = -1 for i < i_pivot, and s_i = 1 for i >= i_pivot,
  // but this may occasionally not be the best choice in the presence of
  // a significant negative offset d0.
  //
  // The results, m, b, and d0, are converted to the "slope" and "intercept"
  // of the cluster geometery, where slope = 1/m and intercept = -b/m.
  // d0 is simply converted to time units to give t0, using the asymptotic
  // drift velocity.

  Int_t err = SetupCoordinates();
  if( err )
    return err;

  //--- Perform 3-parameter fits for different sign coefficients
  //
  // We make some simplifying assumptions:
  // - The first wire of the cluster always has negative drift distance.
  // - The last wire always has positive drift.
  // - The sign flips exactly once from - to + somewhere in between

  // FIXME: only flip pivot sign; more doesn't make sense
  FitRes_t best;
  for( Int_t ipivot = 0; ipivot < GetSize()-1; ++ipivot ) {
    if( ipivot != 0 ) {
      assert( fCoord[ipivot].s == 1 );
      fCoord[ipivot].s = -1;
    }
    // Do the fit
    FitRes_t thisRes;
    Int_t err = Linear3ParamFit( fCoord, thisRes );
    if( !err && fFitOK && thisRes.chi2 < best.chi2 )
      best = thisRes;
  }
  if( best.chi2 >= kBig )
    return -1;

  AcceptResults( best );
  return 0;
}


//_____________________________________________________________________________
Int_t THaVDCClusterFitter::FitNLTrack()
{
  // Mike Paolone's non-linear 3-parameter fit

  // Perform a non-linear fit on drift times.  Calculates slope, intercept,
  // t0, and errors.
  // This fitting routine takes y = m*f(t - t0) +b, where f(t) is parameterized
  // as an Nth Order Polynomial.

  // X = Drift Times
  // Y = Wire Positions

  //  cout << "beginning Fit" << endl;

  const Int_t itMax = 10;  //total number of iterations
  const Int_t nSignCombos = 4; //Number of different sign combinations

  fFitOK = false;
  if( GetSize() < 3 )
    return 1;  // Too few hits to get meaningful results
             // Do keep current values of slope and intercept

  Double_t m;  //, sigmaM;  // Slope, St. Dev. in slope
  Double_t b;  //, sigmaB;  // Intercept, St. Dev in Intercept
  Double_t t0; //, sigmaT0;  // Time offset, St Dev
  // Double_t sigmaY;     // St Dev in delta Y values
  Double_t PosNeg; // +1 or -1 depending on timing and pivot wire.
  Double_t drftVel;
  Double_t err_smallest;
  Int_t it_smallest;

  // Double_t a23 = 0.0, a22 = 1.06E-4, a21 = 1.3E-3, a20 = -4.2E-4, a10 = 2.12E-3;  // We need these if the more complicated fit is attempted.

  Bool_t iter;  //Continue iteration?
  Int_t it; //Iteration counter

  //  Double_t* xArr = new Double_t[GetSize()];
  // Double_t* xDist = new Double_t[GetSize()];
  // Double_t* xDistTDCcor = new Double_t[GetSize()];
  //  Double_t* yArr = new Double_t[GetSize()];
  //  Double_t* polArr = new Double_t[GetSize()];
  // Double_t* dydm = new Double_t[GetSize()];
  // Double_t* dydb = new Double_t[GetSize()];
  // Double_t* dydt0 = new Double_t[GetSize()];


  Double_t bestFit = kBig;

  // Find the index of the pivot wire and copy distances into local arrays
  // Note that as the index of the hits is increasing, the position of the
  // wires is decreasing

  Int_t pivotNum = 0;

  Double_t slope_guess = fSlope;

  drftVel = fPlane->GetDriftVel();
  for (int i = 0; i < GetSize(); i++) {
    if (fHits[i] == fPivot) {
      //FIXME: make pivotNum a member variable?
      pivotNum = i;
    }
  //   xArr[i] = fHits[i]->GetTime();
  //   yArr[i] = fHits[i]->GetPos();
  }

  Double_t pvttime = fPivot->GetTime();

  Double_t mi;   //
  Double_t bi;   //   Initial guesses for m,b and t0.
  Double_t t0i;  //

  TMatrixD J(GetSize(),3); //Jacobian
  TVectorD D(3); //  Delta vector: Iterative change to fit parameters;
  TVectorD r(GetSize()); // Residuals vector

  TMatrixD JT(3,GetSize()); // Transpose(Jacobian)
  TMatrixD JTJ(3,3); // Transpose(Jacobian)*Jacobian
  TVectorD JTr(3); // Transpose(J)*r

  Double_t err,err_old;
  Double_t err_tot[nSignCombos];
  Double_t m_tot[nSignCombos];
  Double_t b_tot[nSignCombos];
  Double_t t0_tot[nSignCombos];
  Double_t err_it[itMax+1], m_it[itMax+1], b_it[itMax+1], t0_it[itMax+1];

  for (int i = 0; i < nSignCombos; i++) {
      iter = 1;


      mi = slope_guess;
      bi = -1.0;
      t0i = -1.0E-9;

      it = 0;
      while(iter){
	it++;

	for (int j = 0; j < GetSize(); j++) {

	  if(j == 0 && it == 1){
	    m = mi;
	    b = bi;
	    t0 = t0i;
	    err = 0.0;
	  }
	  if(j <= pivotNum - 2 + i){
	    PosNeg = 1.0;
	  } else {
	    PosNeg = -1.0;
	  }



	  /* // attempt to fit both the time to distance conversion (slope dependent) and the t0 and intercept all at once.

	  xDist[j] = drftVel*(xArr[j] - t0);
	  xDistTDCcor[j] = xDist[j]*(a23*pow(m, -3) + a22*pow(m,-2) + a21/m + a20)/a10; // m*(vel*(time - t0))*(a23m^-3 + a22m^-2 + a21m^-1 + a20)/a10
	  dydb[j] = 1.0;
	  if(xDist[j] < a10){
	    polArr[j] = m*PosNeg*(xDist[j] + xDistTDCcor[j] + fTimeCorrection) + b;
	    dydm[j] = PosNeg*(drftVel*(xArr[j] - t0)*(1 - 2*a23*pow(m,-3) - a22*pow(m,-2) + a20)/a10 + fTimeCorrection);
	    dydt0[j] = -PosNeg*m*drftVel*(1 + (a23*pow(m, -3) + a22*pow(m,-2) + a21/m + a20)/a10);
	  } else {
	    polArr[j] = m*PosNeg*(xDist[j] + xDistTDCcor[j]*a10/xDist[j] + fTimeCorrection) + b;
	    dydm[j] = PosNeg*(xDist[j] + (-2*a23*pow(m,-3) - a22*pow(m,-2) + a20) + fTimeCorrection);
	    dydt0[j] = -m*PosNeg*drftVel;
	  }

	  */

	  // Fit using the predefined time to distance conversion


	  //	  xArr[j] = PosNeg*(fHits[j]->GetDist() + fTimeCorrection - t0*drftVel);

	  //FIXME: ooops, this isn't the non-linear fit we want!
	  Double_t x = PosNeg*(fHits[j]->GetDist() + fTimeCorrection - t0*drftVel);
	  Double_t y = fHits[i]->GetPos();

	  //polArr[j] = m*(xArr[j]) +b;
	  //	  dydm[j] = xArr[j];
	  // dydm[j] = x;
	  // dydb[j] = 1;
	  // dydt0[j] = -m*drftVel*PosNeg;

	  //	  r(j) = yArr[j] - polArr[j];
	  // J(j,0) = dydm[j];
	  // J(j,1) = dydb[j];
	  // J(j,2) = dydt0[j];

	  Double_t yfit = m*x +b;
	  r(j) = y - yfit;
	  J(j,0) = x;
	  J(j,1) = 1;
	  J(j,2) = -m*drftVel*PosNeg;

	}

	if(it == 1){
	  err_old = kBig;
	}else{
	  err_old = err;
	}

	err = 0;
	for(Int_t jj =0; jj < GetSize(); jj++){
	  err += pow(r(jj),2);
	}
	m_it[it] = m;
	b_it[it] = b;
	t0_it[it] = t0;
	err_it[it] = err;


	if(TMath::Abs((err_old - err)/err) < 0.0001){



	  err_tot[i] = err_it[it];
	  m_tot[i] = m_it[it];
	  b_tot[i] = b_it[it];
	  t0_tot[i] = t0_it[it];
	  iter = 0;
	  it = 0;
	} else if(it > itMax){
	  err_smallest = kBig;
	  bool found = false;
	  for(Int_t jj =1; jj < it; jj++){
	    if(err_it[jj] < err_smallest){
	      it_smallest = jj;
	      found = true;
	    }
	  }

	  if( found ) {
	    err_tot[i] = err_it[it_smallest];
	    m_tot[i] = m_it[it_smallest];
	    b_tot[i] = b_it[it_smallest];
	    t0_tot[i] = t0_it[it_smallest];
	  } else {
	    // happens if all err_it are nan or inf
	    err_tot[i] = kBig;
	    m_tot[i] = kBig;
	    b_tot[i] = kBig;
	    t0_tot[i] = kBig;
	    // probably should set fFitOK = false and bail
	  }

	  iter = 0;
	  it = 0;

	} else {

	  iter = 1;
	  Bool_t DecompOK;
	  JT.Transpose(J);
	  JTJ = JT*J;
	  JTr = JT*r;
	  TDecompSVD lu(JTJ);
	  //  TDecompQRH lu(JTJ, 1.E-40);  // Decompose for linear solution.
	                               // Using the QRH decomposition instead of the SVD decomposition
                                       // because it is about ~2.5x faster; however the tolorance must
                                       // be set very low: ~ 1E-40.  EDIT: with the tolerance set low
	                               // the speed is about the same.


	  D = lu.Solve(JTr,DecompOK);  // set (trans(J)*J)*D = trans(J)*r and solve for D.
	  m += D(0);
	  b += D(1);
	  t0 += D(2);
	}



      }
      if( err_tot[i] < bestFit || i ==0  ){
	  fLocalSlope = m_tot[i];
	  fInt = b_tot[i];

	// 9/15 SPR - This is a dirty hack which
	// is better than misfitting
	if( t0_tot[i] < pvttime ){
	  fT0 = t0_tot[i];
	} else {
	  fT0 = pvttime;
	}

	bestFit = err_tot[i];
      }
  }

    // Pick the best value

  // calculate chi2 for the track given this slope and intercept
  Double_t chi2 = bestFit;
  Int_t nhits = GetSize();

  //  CalcChisquare(chi2,nhits);
  fChi2 = chi2;
  fNDoF = nhits-3;

  fFitOK = true;

  // delete[] xArr;
  // delete[] yArr;
  // delete[] xDist;
  // delete[] xDistTDCcor;
  // delete[] polArr;
  // delete[] dydm;
  // delete[] dydb;
  // delete[] dydt0;
  return 0;
}

//_____________________________________________________________________________
chi2_t THaVDCClusterFitter::CalcChisquare( Double_t slope, Double_t icpt,
					   Double_t d0 ) const
{
  Int_t npt = 0;
  Double_t chi2 = 0;
  for( int j = 0; j < GetSize(); ++j ) {
    Double_t x  = fCoord[j].x;
    Double_t y  = fCoord[j].s * fCoord[j].y;
    Double_t w  = fCoord[j].w;
    Double_t yp = x*slope + icpt + d0*fCoord[j].s;
    if( w < 0 ) continue;
    Double_t d  = y-yp;
    chi2       += d*d*w;
    ++npt;
  }
  return make_pair( chi2, npt );
}

//_____________________________________________________________________________
void THaVDCClusterFitter::AcceptResults( const FitRes_t& res )
{
  // Accept fit results given in 'res'

  fChi2       = res.chi2;
  fNDoF       = res.ndof;
  fLocalSlope = res.coef[0];
  fInt        = res.coef[1];
  fT0         = (res.n == 3 ) ? res.coef[2] : 0;
  //FIXME: do we really need these?
  fSigmaSlope = res.cov[0][0];
  fSigmaInt   = res.cov[1][1];
  fSigmaT0    = (res.n == 3 ) ? res.cov[2][2] : 0;

  //FIXME: convert to cluster definition of slope/intercept here?
#if 0
  // FIXME: this converts to the "cluster definition" of slope/intercept
  res.m   =  1./a;
  res.b   = -b/a;
  res.t0  = 0;
  res.dm  = res.m * res.m * TMath::Sqrt( sigma_a2 );
  res.db  = TMath::Sqrt( sigma_b2 + b*b/(a*a)*sigma_a2 - 2*b/a*sigma_ab )/a;
  res.dt0 = 0;

  // Rotate the coordinate system to match the VDC definition of "slope"
  fLocalSlope = 1.0/fLocalSlope;  // 1/m
  fInt   = -fInt * fLocalSlope;   // -b/m
  if( fPlane ) {
    fT0      /= fPlane->GetDriftVel();
    fSigmaT0 /= fPlane->GetDriftVel();
  }

#endif

  fSlope      = fLocalSlope;   // current best guess for the global slope

  fFitOK      = true;
}

//_____________________________________________________________________________
ClassImp(THaVDCClusterFitter)
