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

using namespace std;

//FIXME put into VDC namespace
const Vhit_t::size_type kDefaultNHit = 16;

//_____________________________________________________________________________
THaVDCClusterFitter::THaVDCClusterFitter( THaVDCPlane* owner )
  : THaVDCCluster(owner)
{
  // Constructor

  fCoord.reserve(kDefaultNHit);
}

//_____________________________________________________________________________
THaVDCClusterFitter::THaVDCClusterFitter( const Vhit_t& hits,
					  THaVDCPlane* owner )
  : THaVDCCluster(owner)
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
void THaVDCClusterFitter::EstTrackParameters()
{
  // Estimate Track Parameters
  // Calculates pivot wire and uses its position as position of cluster
  // Estimates the slope based on the distance between the first and last
  // wires to be hit in the U (or V) and detector Z directions

  fFitOK = false;
  if( GetSize() == 0 )
    return;

  // Find pivot
  Double_t time = 0, minTime = 1000;  // drift-times in seconds
  for (int i = 0; i < GetSize(); i++) {
    time = fHits[i]->GetTime();
    if (time < minTime) { // look for lowest time
      minTime = time;
      fPivot = fHits[i];
    }
  }

  // Now set intercept
  fInt = fPivot->GetPos();

  // Now find the approximate slope
  //   X = Drift Distance (m)
  //   Y = Position of Wires (m)
  if( GetSize() > 1 ) {
    Double_t conv = fPlane->GetDriftVel();  // m/s
    Double_t dx = conv * (fHits[0]->GetTime() + fHits[GetSize()-1]->GetTime());
    Double_t dy = fHits[0]->GetPos() - fHits[GetSize()-1]->GetPos();
    fSlope = dy / dx;
  } else
    fSlope = 1.0;

  fFitOK = true;
}

//_____________________________________________________________________________
void THaVDCClusterFitter::ConvertTimeToDist( Double_t t0 )
{
  // Convert TDC Times in wires to drift distances

  //Do conversion for each hit in cluster
  for (int i = 0; i < GetSize(); i++)
    fHits[i]->ConvertTimeToDist(fSlope,t0);
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
void THaVDCClusterFitter::FitSimpleTrack( Bool_t weighted )
{
  // Perform linear fit on drift times. Calculates slope, intercept, and errors.
  // Does not assume the uncertainty is the same for all hits.
  //
  // Assume t0 = 0.

  // For this function, the final results is (m,b) in  Y = m X + b
  //   X = Drift Distance
  //   Y = Position of Wires

  fFitOK = false;
  if( GetSize() < 3 ) {
    return;  // Too few hits to get meaningful results
             // Do keep current values of slope and intercept
  }

  Double_t m, sigmaM;  // Slope, St. Dev. in slope
  Double_t b, sigmaB;  // Intercept, St. Dev in Intercept

  fCoord.clear();

  Double_t bestFit = 0.0;

  // Find the index of the pivot wire and copy distances into local arrays
  // Note that as the index of the hits is increasing, the position of the
  // wires is decreasing

  Int_t pivotNum = 0;
  for (int i = 0; i < GetSize(); i++) {

    // In order to take into account the varying uncertainty in the
    // drift distance, we will be working with the X' and Y', and
    //       Y' = F + G X'
    //  where Y' = X, and X' = Y  (swapping it around)
    if (fHits[i] == fPivot) {
      pivotNum = i;
    }

    Double_t x = fHits[i]->GetPos();
    Double_t y = fHits[i]->GetDist() + fTimeCorrection;
    Double_t w;
    if( weighted ) {
      w = fHits[i]->GetdDist();
      // the hit will be ignored if the uncertainty is < 0
      if (w>0)
	w = 1./(w*w); // sigma^-2 is the weight
      else
	w = -1.;
    } else {
      w = 1.0;
    }
    fCoord.push_back( FitCoord_t(x,y,w) );
  }

  const Int_t nSignCombos = 2; //Number of different sign combinations
  for (int i = 0; i < nSignCombos; i++) {
    Double_t F, sigmaF2;  // intermediate slope and St. Dev.**2
    Double_t G, sigmaG2;  // intermediate intercept and St. Dev.**2
    Double_t sigmaFG;     // correlated uncertainty

    Double_t sumX  = 0.0;   //Positions
    Double_t sumXW = 0.0;
    Double_t sumXX = 0.0;
    Double_t sumY  = 0.0;   //Drift distances
    Double_t sumXY = 0.0;
    Double_t sumYY = 0.0;
    Double_t sumXXW= 0.0;

    Double_t W = 0.0;
    Double_t WW= 0.0;

    if (i == 0)
      for (int j = pivotNum+1; j < GetSize(); j++)
	fCoord[j].y *= -1;
    else if (i == 1)
      fCoord[pivotNum].y *= -1;

    for (int j = 0; j < GetSize(); j++) {
      Double_t x = fCoord[j].x;   // Position of wire
      Double_t y = fCoord[j].y;   // Distance to wire
      Double_t w = fCoord[j].w;

      if (w <= 0) continue;
      W     += w;
      WW    += w*w;
      sumX  += x * w;
      sumXW += x * w * w;
      sumXX += x * x * w;
      sumXXW+= x * x * w * w;
      sumY  += y * w;
      sumXY += x * y * w;
      sumYY += y * y * w;
    }

    // Standard formulae for linear regression (see Bevington)
    Double_t Delta = W * sumXX - sumX * sumX;

    F  = (sumXX * sumY - sumX * sumXY) / Delta;
    G  = (W * sumXY - sumX * sumY) / Delta;
    sigmaF2 = ( sumXX / Delta );
    sigmaG2 = ( W / Delta );
    sigmaFG = ( sumXW * W * sumXX - sumXXW * W * sumX
		- WW * sumX * sumXX + sumXW * sumX * sumX ) / (Delta*Delta);

    m  =   1/G;
    b  = - F/G;

    sigmaM = m * m * TMath::Sqrt( sigmaG2 );
    sigmaB = TMath::Sqrt( sigmaF2/(G*G) + F*F/(G*G*G*G)*sigmaG2 - 2*F/(G*G*G)*sigmaFG);

    // calculate chi2 for the track given this slope and intercept
    chi2_t chi2 = CalcChisquare( m, b, 0 );

    // scale the uncertainty of the fit parameters based upon the
    // quality of the fit. This really should not be necessary if
    // we believe the drift-distance uncertainties
    // sigmaM *= chi2/(nhits - 2);
    // sigmaB *= chi2/(nhits - 2);

    // Pick the best value
    if (i == 0 || chi2.first < bestFit) {
      bestFit     = fChi2 = chi2.first;
      fNDoF       = chi2.second - 2;
      fLocalSlope = m;
      fInt        = b;
      fSigmaSlope = sigmaM;
      fSigmaInt   = sigmaB;
    }
  }

  fFitOK = true;
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


  fFitOK = false;
  if( GetSize() < 4 ) {
    return -1;  // Too few hits to get meaningful results
                // Do keep current values of slope and intercept
  }

  Double_t m, sigmaM;   // Slope, St. Dev. in slope
  Double_t b, sigmaB;   // Intercept, St. Dev in Intercept
  Double_t d0, sigmaD0;

  sigmaM = sigmaB = sigmaD0 = 0;

  fCoord.clear();

  //--- Copy hit data into local arrays
  Int_t ihit, incr, ilast = GetSize()-1;
  // Ensure that the first element of the local arrays always corresponds
  // to the wire with the smallest x position
  bool reversed = ( fPlane->GetWSpac() < 0 );
  if( reversed ) {
    ihit = ilast;
    incr = -1;
  } else {
    ihit = 0;
    incr = 1;
  }
  for( Int_t i = 0; i < GetSize(); ihit += incr, ++i ) {
    assert( ihit >= 0 && ihit < GetSize() );
    Double_t x = fHits[ihit]->GetPos();
    Double_t y = fHits[ihit]->GetDist() + fTimeCorrection;

    Double_t wt = fHits[ihit]->GetdDist();
    if( wt>0 )   // the hit will be ignored if the uncertainty is <= 0
      wt = 1./(wt*wt);
    else
      wt = -1.;

    fCoord.push_back( FitCoord_t(x,y,wt) );

    assert( i == 0 || fCoord[i-1].x < fCoord[i].x );
  }

  Double_t bestFit = 0.0;

  //--- Perform 3-parameter for different sign coefficients
  //
  // We make some simplifying assumptions:
  // - The first wire of the cluster always has negative drift distance.
  // - The last wire always has positive drift.
  // - The sign flips exactly once from - to + somewhere in between
  fCoord[0].s = -1;
  for( Int_t i = 1; i < GetSize(); ++i )
    fCoord[i].s = 1;

  for( Int_t ipivot = 0; ipivot < ilast; ++ipivot ) {
    if( ipivot != 0 )
      fCoord[ipivot].s *= -1;

    // Do the fit
    Linear3DFit( m, b, d0 );

    // calculate chi2 for the track given this slope,
    // intercept, and distance offset
    chi2_t chi2 = CalcChisquare( m, b, d0 );

    // scale the uncertainty of the fit parameters based upon the
    // quality of the fit. This really should not be necessary if
    // we believe the drift-distance uncertainties
//     sigmaM *= chi2/(nhits - 2);
//     sigmaB *= chi2/(nhits - 2);

    // Pick the best value
    if( ipivot == 0 || chi2.first < bestFit ) {
      bestFit     = fChi2 = chi2.first;
      fNDoF       = chi2.second - 3;
      fLocalSlope = m;
      fInt        = b;
      fT0         = d0;
      fSigmaSlope = sigmaM;
      fSigmaInt   = sigmaB;
      fSigmaT0    = sigmaD0;
    }
  }

  // Rotate the coordinate system to match the VDC definition of "slope"
  fLocalSlope = 1.0/fLocalSlope;  // 1/m
  fInt   = -fInt * fLocalSlope;   // -b/m
  if( fPlane ) {
    fT0      /= fPlane->GetDriftVel();
    fSigmaT0 /= fPlane->GetDriftVel();
  }

  fFitOK = true;

  return 0;
}

//_____________________________________________________________________________
void THaVDCClusterFitter::Linear3DFit( Double_t& m, Double_t& b, Double_t& d0 ) const
{
  // 3-parameter fit
//     Double_t F, sigmaF2;  // intermediate slope and St. Dev.**2
//     Double_t G, sigmaG2;  // intermediate intercept and St. Dev.**2
//     Double_t sigmaFG;     // correlated uncertainty

  Double_t sumX   = 0.0;   //Positions
  Double_t sumXX  = 0.0;
  Double_t sumD   = 0.0;   //Drift distances
  Double_t sumXD  = 0.0;
  Double_t sumS   = 0.0;   //sign vector
  Double_t sumSX  = 0.0;
  Double_t sumSD  = 0.0;
  Double_t sumSDX = 0.0;

  //     Double_t sumXW = 0.0;
  //     Double_t sumXXW= 0.0;

  Double_t sumW  = 0.0;
  //     Double_t sumWW = 0.0;
  //     Double_t sumDD = 0.0;


  for (int j = 0; j < GetSize(); j++) {
    Double_t x = fCoord[j].x;   // Position of wire
    Double_t d = fCoord[j].y;   // Distance to wire
    Double_t w = fCoord[j].w;   // Weight/error of distance measurement
    Int_t    s = fCoord[j].s;   // Sign of distance

    if (w <= 0) continue;

    sumX   += x * w;
    sumXX  += x * x * w;
    sumD   += d * w;
    sumXD  += x * d * w;
    sumS   += s * w;
    sumSX  += s * x * w;
    sumSD  += s * d * w;
    sumSDX += s * d * x * w;
    sumW   += w;
    //       sumWW  += w*w;
    //       sumXW  += x * w * w;
    //       sumXXW += x * x * w * w;
    //       sumDD  += d * d * w;
  }

  // Standard formulae for linear regression (see Bevington)
  Double_t Delta =
    sumXX   * ( sumW  * sumW - sumW * sumS  ) -
    sumX    * ( sumX  * sumW - sumX * sumS  );

  m =
    sumSDX  * ( sumW  * sumW - sumW * sumS  ) -
    sumSD   * ( sumX  * sumW - sumW * sumSX ) +
    sumD    * ( sumX  * sumS - sumW * sumSX );

  b =
    -sumSDX * ( sumX  * sumW - sumX * sumS  ) +
    sumSD   * ( sumXX * sumW - sumX * sumSX ) -
    sumD    * ( sumXX * sumS - sumX - sumSX );

  d0 = ( sumD - sumSD ) * ( sumXX * sumW - sumX * sumX );

  m  /= Delta;
  b  /= Delta;
  d0 /= Delta;


  //     F  = (sumXX * sumD - sumX * sumXD) / Delta;
  //     G  = (sumW * sumXD - sumX * sumD) / Delta;
  //     sigmaF2 = ( sumXX / Delta );
  //     sigmaG2 = ( sumW / Delta );
  //     sigmaFG = ( sumXW * sumW * sumXX - sumXXW * sumW * sumX
  // 		- sumWW * sumX * sumXX + sumXW * sumX * sumX ) / (Delta*Delta);

  //     m  =   1/G;
  //     b  = - F/G;

  //     sigmaM = m * m * TMath::Sqrt( sigmaG2 );
  //     sigmaB = TMath::Sqrt( sigmaF2/(G*G) + F*F/(G*G*G*G)*sigmaG2 - 2*F/(G*G*G)*sigmaFG);

}

//_____________________________________________________________________________
void THaVDCClusterFitter::FitNLTrack()
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
    return;  // Too few hits to get meaningful results
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
  // for (int i = 0; i < GetSize(); i++) {
  //   if (fHits[i] == fPivot) {
  //     pivotNum = i;
  //   }
  //   xArr[i] = fHits[i]->GetTime();
  //   yArr[i] = fHits[i]->GetPos();
  // }

  Double_t pvttime = fHits[pivotNum]->GetTime();

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
ClassImp(THaVDCClusterFitter)
