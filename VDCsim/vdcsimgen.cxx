// Modified version of hsimple.cxx for VDC simulation
// Ken Rossato 6/3/04
// Modified by Amy Orsborn 6/7/05

// @(#)root/test:$Name$:$Id$
// Author: Rene Brun   19/08/96

// UNITS: distances: m
//        times: ns
//        angles: tan angle
//        mass: 1 (momentum == velocity)

#include "THaVDCSim.h"

int vdcsimgen(TString filename, double deltaTsigma);
void getargs(double& deltaTsigma, TString& filename, int argc, char*argv[]);
void usage();

#ifndef __CINT__
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#include "TVector3.h"
#include "TString.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

//______________________________________________________________________________
int main(int argc, char *argv[])
{
   //set variable defaults
  TString filename = "vdctracks.root"; 
  double deltaTsigma = 4.5;

   getargs(deltaTsigma, filename, argc, argv);
   return vdcsimgen(filename, deltaTsigma);
}
#endif

void usage()
{
  printf("%s \n", "Usage: %s [-s output file name] [-n noise]");
  puts(" -s <output file name.> Default is 'vcdtracks.root'");
  puts(" -n <noise> Simulated noise sigma value (>0). Default is 4.5");
  exit(1);
}

void getargs( double& deltaTsigma, TString& filename,  int argc, char **argv )
{
  while (argc-- >1){
    char *opt = *++argv;
    if(*opt == '-'){
      while (*++opt != '\0'){
	switch (*opt){
	case 's':
	   if(!*++opt){
	    if (argc-- < 1)
	      usage();
	    opt = *++argv;
	   }
	  filename = opt;
	  opt = "?";
	  break;
	case 'n':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  deltaTsigma = atof(opt);
	  if( deltaTsigma < 0.0 )
	    usage();
	  opt = "?";
	  break;
	default:
	  usage();
	}
      }
    }
  }
  }

int vdcsimgen(TString filename, double deltaTsigma)
{
  // Create a new ROOT binary machine independent file.
  // This file is now becoming the current directory.
 TFile hfile(filename,"RECREATE","ROOT file for VDC simulation");

  // Define the appropriate range for incident tracks
  const Float_t x1(-.8), x2(.9), ymean(0), ysigma(.01), z0(-1);
  const Float_t pthetamean(TMath::Pi()/4.0), 
    pthetasigma(atan(1.1268)-TMath::Pi()/4.0), pmag0(1), 
    pphimean(0), pphisigma(atan(.01846)), deltaTmean(0);

  // Define the height of the sense wires
  THaVDCSimConditions *s = new THaVDCSimConditions;
  s->set(s->wireHeight, 0, .026, .335, .026+.335);
  s->set(s->wireUVOffset, 0,0,.335/sqrt(2.0),.335/sqrt(2.0));
  s->numWires = 368;
  s->deltaTsigma = deltaTsigma;
  cout << deltaTsigma << endl;

  // driftVelocity is in m/ns
  s->set(s->driftVelocities, .0000504, .0000511, .0000509, .0000505);

  //hardcode prefixes for wire planes.
  s->Prefixes[0] = "L.vdc.u1";
  s->Prefixes[1] = "L.vdc.v1";
  s->Prefixes[2] = "L.vdc.u2";
  s->Prefixes[3] = "L.vdc.v2";

  float timeOffsets[4*s->numWires];
 
  //get time offsets from the file (default is 20030415/L.vdc.dat)
  for(int j=0; j<4; j++){
    Int_t getoffset = s->ReadDatabase(j, timeOffsets, s->numWires); 
    if(getoffset == 1) 
      cout << "Error Reading Database" << endl; 
  }

  //define correlation values for convertion from time to distance
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
  TH1F *hnumwiresU1 = new TH1F("hnumwiresU1", "Number of wires triggered in U1 Plane", 10, 0, 10);
  TH1F *hnumwiresV1 = new TH1F("hnumwiresV1", "Number of wires triggered in V1 Plane", 10, 0, 10);
  TH1F *hnumwiresU2 = new TH1F("hnumwiresU2", "Number of wires triggered in U2 Plane", 10, 0, 10);
  TH1F *hnumwiresV2 = new TH1F("hnumwiresV2", "Number of wires triggered in V2 Plane", 10, 0, 10);
  TH2F *horigin = new TH2F("horigin","Origin Y vs. X",100,-1.2,1.2,100,-.1,.1);
  TH1F *hdrift = new TH1F("hdrift","Drift Time", 100, 1000, 2000);
  TH1F *hdriftNoise = new TH1F ("hdriftNoise", "Drift Time (With Noise)", 100, 1000, 2000);
  TH2F *hdrift2 = new TH2F("hdrift2", "Drift Time vs Num Wires in Cluster",
			   6, 3, 9, 100, 1000, 2000);
  TH1F *hdeltaT4 = new TH1F ("hdeltaT4", "Relative Time (4 hits)", 200, -100, 100);
  TH1F *hdeltaTNoise4 = new TH1F ("hdeltaTNoise4", "Relative Time (4 hits w/ Noise)", 200, -100, 100);
  TH1F *hdeltaT5 = new TH1F ("hdeltaT5", "Relative Time (5 hits)", 200, -100, 100);
  TH1F *hdeltaTNoise5 = new TH1F ("hdeltaTNoise5", "Relative Time (5 hits w/ Noise)", 200, -100, 100); 
  TH1F *hdeltaT6 = new TH1F ("hdeltaT6", "Relative Time (6 hits)", 200, -100, 100);
  TH1F *hdeltaTNoise6 = new TH1F ("hdeltaTNoise6", "Relative Time (6 hits w/ Noise)", 200, -100, 100);

  THaVDCSimEvent *track = new THaVDCSimEvent;
  TTree *tree = new TTree("tree","VDC Track info");
  TBranch *trackBranch = tree->Branch("track", "THaVDCSimEvent", &track); 

  Int_t event_num = 0;

  //create 10000 simulated tracks
  for ( Int_t i=0; i<10000; i++) 
   {
    track->event_num = event_num++;

    // create randomized origin and momentum vectors
    track->origin.SetXYZ(gRandom->Rndm(1)*(x2-x1) + x1,gRandom->Gaus(ymean, ysigma),z0);
    track->momentum.SetXYZ(0,0,pmag0);
    track->momentum.SetTheta(gRandom->Gaus(pthetamean,pthetasigma));
    track->momentum.SetPhi(gRandom->Gaus(pphimean, pphisigma));
    
    //Fill histogram with origin position
    horigin->Fill(track->X(), track->Y());

    //run through each plane (u1, v1, u2, v2)
    for (Int_t j=0; j < 4; j++) 
    {    
      // figuring out which wires are hit
      
      //calculate time it takes to hit wire plane and the position at which it hits
      Float_t t = (s->wireHeight[j]-track->origin.Z())/track->momentum.Z();
      TVector3 position = track->origin + t*track->momentum;
 
      // Get it into (u,v,z) coordinates for u plane
      // (v,-u,z) coordinates for v plane
      if (j%2 == 0)
	position.RotateZ(TMath::Pi()/4.0);
      else
	position.RotateZ(-TMath::Pi()/4.0);
	
      
      //find out which range of wires are hit
      
      Float_t tanThetaPrime = track->Theta()*sqrt(2.0);
      Double_t a1 = 0.0, a2 = 0.0;
      
      // Find the values of a1 and a2 by evaluating the proper polynomials
      // a = A_3 * x^3 + A_2 * x^2 + A_1 * x + A_0
      for (Int_t i = 3; i >= 1; i--) 
      {
	a1 = tanThetaPrime * (a1 + fA1tdcCor[i]);
	a2 = tanThetaPrime * (a2 + fA2tdcCor[i]);
      }
      a1 += fA1tdcCor[0];
      a2 += fA2tdcCor[0];
      
      // position in m, 0 = sense wire #0
      Float_t x = position.X() - s->wireUVOffset[j];

      Int_t wirehitFirst, wirehitLast, pivotWire, counter = 0;
      Float_t times[6];
      Float_t timesN[6];  //arrays to hold times when 5 wires hit (to calc. relative time)

      pivotWire = static_cast<Int_t>(x/.00424);
      wirehitFirst = static_cast<Int_t>((x - .026/tanThetaPrime/2)/.00424);
      wirehitLast = static_cast<Int_t>((x + .026/tanThetaPrime/2)/.00424);

      Int_t numwires = wirehitLast - wirehitFirst + 1;

      //loop through each hit wire
      for (Int_t k=wirehitFirst; k<=wirehitLast; k++) 
      {
	//declare wirehit class
	THaVDCSimWireHit *hit = new THaVDCSimWireHit;

	hit->wirenum = k;

	// distance from crossing point to wire "k"
	Float_t d = x - (k+.5)*.00424;
	// Actual drift distance
	Float_t d0 = TMath::Abs(d)*tanThetaPrime;

	// Inversion of THaVDCAnalyticTTDConv::ConvertTimeToDist
	if (d0 > a1 + a2) 
	{
	  d0 -= a2;
	}
	else 
	{
	  d0 /= (1+a2/a1);
	}

	hit->timeN = (timeOffsets[k+j*s->numWires] - 2*(t 
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
	
	hit->time = hit->timeN + gRandom->Gaus(deltaTmean, deltaTsigma); 
	  //time with additional noise to account for random walk nature of ions
	  //mean & sigma used based on experimental results (Fissum et all paper, fig 13)

	//if 4,5, or 6 wires hit, keep track of each time 
	if(numwires >= 4 && numwires <= 6)
	{
	  times[counter] = hit->time;
	  timesN[counter] = hit->timeN;
	}
	
	counter++;  //increase counter

	if (hit->wirenum < 0 || hit->wirenum > 367) 
	{
	  delete hit;
	  continue;
	}

	//fill histograms for v1 plane
	if (j == 1) 
	{
	  hdrift->Fill((hit->timeN));
	  hdriftNoise->Fill((hit->time));
	  hdrift2->Fill(wirehitLast-wirehitFirst+1, (hit->timeN));
	  hwire->Fill(k);
	}

	track->wirehits[j]->Add(hit);

      } //closes loop going through each hit wire

      if(j == 0)      
	hnumwiresU1->Fill(wirehitLast - wirehitFirst + 1);
      else if(j == 1)
	hnumwiresV1->Fill(wirehitLast - wirehitFirst + 1);
      else if(j == 2)
	hnumwiresU2->Fill(wirehitLast - wirehitFirst + 1);
      else
	hnumwiresV2->Fill(wirehitLast - wirehitFirst +1);

      // calculate relative time and store in histograms for 4, 5, and 6 hits
      if(numwires == 4)
        {
	  hdeltaTNoise4->Fill((times[0] - times[1]) - (times[3] - times[2]));
	  hdeltaT4->Fill((timesN[0] - timesN[1]) - (timesN[3] - timesN[2]));
        }
      else if(numwires == 5)
	{
	  hdeltaTNoise5->Fill((times[0] - times[1]) - (times[4] - times[3]));
	  hdeltaT5->Fill((timesN[0] - timesN[1]) - (timesN[4] - times[3]));
	}
      else if(numwires == 6)
	{
	  hdeltaTNoise6->Fill((times[0] - times[1]) - (times[5] - times[4]));
	  hdeltaT6->Fill((timesN[0] - timesN[1]) - (timesN[5] - timesN[4]));
	}

    }//closes loop going through each plane

    //fill the tree
    trackBranch->Fill();
    
    //clean up
    track->Clear();
   } //closes loop going through 10000 trials

  // Save SOME objects in this file
  tree->Write();
  s->Write("s");

  hwire->Write();
  hnumwiresU1->Write();
  hnumwiresV1->Write();
  hnumwiresU2->Write();
  hnumwiresV2->Write();
  horigin->Write();
  hdrift->Write();
  hdrift2->Write();
  hdriftNoise->Write();
  hdeltaT4->Write();
  hdeltaTNoise4->Write();
  hdeltaT5->Write();
  hdeltaTNoise5->Write();
  hdeltaT6->Write();
  hdeltaTNoise6->Write();

  // Close the file.
  hfile.Close();

  return 0;
}
