// Modified version of hsimple.cxx for VDC simulation
// Ken Rossato 6/3/04

// @(#)root/test:$Name$:$Id$
// Author: Rene Brun   19/08/96

// UNITS: distances: m
//        times: ns
//        angles: tan angle
//        mass: 1 (momentum == velocity)

#include "THaVDCSim.h"

int vdcsimgen();

#ifndef __CINT__
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#include "TVector3.h"

//______________________________________________________________________________
int main()
{
  return vdcsimgen();
}
#endif

int vdcsimgen()
{
  // Create a new ROOT binary machine independent file.
  // Note that this file may contain any kind of ROOT objects, histograms,
  // pictures, graphics objects, detector geometries, tracks, events, etc..
  // This file is now becoming the current directory.
  TFile hfile("vdctracks.root","RECREATE","ROOT file for VDC simulation");

  // Define the appropriate range for incident tracks

  const Float_t x1(-.8), x2(.9), ymean(0), ysigma(.01), z0(-1);
  const Float_t pthetamean(TMath::Pi()/4.0), 
    pthetasigma(atan(1.1268)-TMath::Pi()/4.0), pmag0(1), 
    pphimean(0), pphisigma(atan(.01846));

  // Define the height of the sense wires
  THaVDCSimConditions *s = new THaVDCSimConditions;

  s->set(s->wireHeight, 0, .026, .335, .026+.335);
  s->set(s->wireUVOffset, 0,0,.335/sqrt(2),.335/sqrt(2));
  // driftVelocity is in m/ns (was um/ns)
  s->set(s->driftVelocities, .0000504, .0000511, .0000509, .0000505);

  s->timeOffset = 1771;
  

  Double_t fA1tdcCor[4], fA2tdcCor[4];
  fA1tdcCor[0] = 2.12e-3;
  fA1tdcCor[1] = 0.0;
  fA1tdcCor[2] = 0.0;
  fA1tdcCor[3] = 0.0;
  fA2tdcCor[0] = -4.20e-4;
  fA2tdcCor[1] =  1.3e-3;
  fA2tdcCor[2] = 1.06e-4;
  fA2tdcCor[3] = 0.0;

  // Create some histograms, a profile histogram and an ntuple
  TH1F *hwire   = new TH1F("hwire","Wire Hit Map",100,-5,400);
  TH1F *hnumwires = new TH1F("hnumwires", "Number of wires triggered", 10, 0, 10);
  TH2F *horigin = new TH2F("horigin","Origin Y vs. X",100,-1.2,1.2,100,-.1,.1);
  TH1F *hdrift = new TH1F("hdrift","Drift Time", 100, 1000, 2000);
  TH2F *hdrift2 = new TH2F("hdrift2", "Drift Time vs Num Wires in Cluster",
			   6, 3, 9, 100, 1000, 2000);

  THaVDCSimEvent *track = new THaVDCSimEvent;
  TTree *tree = new TTree("tree","VDC Track info");
  TBranch *trackBranch = tree->Branch("track", "THaVDCSimEvent", &track);

  Int_t event_num = 0;

  for ( Int_t i=0; i<10000; i++) {
    track->event_num = event_num++;

    // We need to create an origin vector and a momentum vector
    track->origin.SetXYZ(gRandom->Rndm(1)*(x2-x1) + x1,
			gRandom->Gaus(ymean, ysigma),
			z0);
    track->momentum.SetXYZ(0,0,pmag0);
    track->momentum.SetTheta(gRandom->Gaus(pthetamean,pthetasigma));
    track->momentum.SetPhi(gRandom->Gaus(pphimean, pphisigma));

    horigin->Fill(track->X(), track->Y());
    for (Int_t j=0; j < 4; j++) {
      // So now we figure out which wires hit
      // Remember there are wires in u1, v1, u2, then v2
      
      // new rule: all time is measured in ns
      Float_t t = (s->wireHeight[j]-track->origin.Z())/track->momentum.Z();
      TVector3 position = track->origin + t*track->momentum;
 
      // Get it into (u,v,z) coordinates for u plane
      // (v,-u,z) coordinates for v plane
      if (j%2 == 0)
	position.RotateZ(TMath::Pi()/4.0);
      else
	position.RotateZ(-TMath::Pi()/4.0);
	
      // and now we have where it hits the plane of wires
      // NEXT we need to find out which RANGE of wires are hit
      
      Float_t tanThetaPrime = track->Theta()*sqrt(2);

      Double_t a1 = 0.0, a2 = 0.0;
      // Find the values of a1 and a2 by evaluating the proper polynomials
      // a = A_3 * x^3 + A_2 * x^2 + A_1 * x + A_0
      
      for (Int_t i = 3; i >= 1; i--) {
	a1 = tanThetaPrime * (a1 + fA1tdcCor[i]);
	a2 = tanThetaPrime * (a2 + fA2tdcCor[i]);
      }
      a1 += fA1tdcCor[0];
      a2 += fA2tdcCor[0];
      


      // position in m, 0 = sense wire #0
      Float_t x = position.X() - s->wireUVOffset[j];

      Int_t wirehitFirst, wirehitLast, pivotWire;

      pivotWire = static_cast<Int_t>(x/.00424);
      wirehitFirst = static_cast<Int_t>((x - .026/tanThetaPrime/2)/.00424);
      wirehitLast = static_cast<Int_t>((x + .026/tanThetaPrime/2)/.00424);

      for (Int_t k=wirehitFirst; k<=wirehitLast; k++) {
	THaVDCSimWireHit *hit = new THaVDCSimWireHit;
	hit->wirenum = k;

	// distance from crossing point to wire "k"
	Float_t d = x - (k+.5)*.00424;
	// Actual drift distance
	Float_t d0 = TMath::Abs(d)*tanThetaPrime;

	// Inversion of THaVDCAnalyticTTDConv::ConvertTimeToDist
	if (d0 > a1 + a2) {
	  d0 -= a2;
	}
	else {
	  d0 /= (1+a2/a1);
	}

	hit->time = static_cast<int>(s->timeOffset - 2*(t 
	  // Correction for the velocity of the track
	  // This correction is negligible, because of high momentum

	  // Ignoring this correction permits momentum to have any
	  // magnitude with no effect on the results
	  //+ d*sqrt(pow(tanThetaPrime,2)+1)/track->momentum.Mag()
	  
	  // Correction for ionization drift velocities
	  + d0/s->driftVelocities[j]));

	// reject data that the noise filter would
	// This shouldn't affect our data at all
	/*
	if (d0 < -.5 * .013 ||
	    d0 > 1.5 * .013) {
	  delete hit;
	  continue;
	}
	*/

	if (hit->wirenum < 0 || hit->wirenum > 367) {
	  delete hit;
	  continue;
	}

	if (j == 1) {
	  hdrift->Fill(static_cast<float>(hit->time));
	  hdrift2->Fill(wirehitLast-wirehitFirst+1, static_cast<float>(hit->time));
	}

	track->wirehits[j].Add(hit);

	if (j == 1) { // fill a simple histogram for v1
	  hwire->Fill(k);
	}
      }
      hnumwires->Fill(wirehitLast - wirehitFirst + 1);
    }

    // finally fill the tree
    trackBranch->Fill();
    
    // clean up
    track->Clear();
  }

  // Save SOME objects in this file
  tree->Write();
  s->Write("s");
  hwire->Write();
  hnumwires->Write();
  horigin->Write();
  hdrift->Write();
  hdrift2->Write();

  // Close the file. Note that this is automatically done when you leave
  // the application.
  hfile.Close();

  return 0;
}
