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
#include <math.h>
#include <fstream>

using namespace std;

int vdcsimgen( THaVDCSimConditions* s );
void getargs( int argc, char*argv[], THaVDCSimConditions* s );
void usage();

//______________________________________________________________________________
int main(int argc, char *argv[])
{
  //declare simulation conditions class
  THaVDCSimConditions *s = new THaVDCSimConditions;

  getargs( argc, argv, s );
  int ret = vdcsimgen( s );

  delete s;
  return ret;
}
#endif

void usage()
{
  printf("%s \n", "Usage: %s [-s output file name] [-n noise]");
  puts(" -s <Output File Name> Default is 'vcdtracks.root'");
  puts(" -f <Textfile Name> Must be of form name.data(). Default is 'trackInfo.data()'");
  puts(" -a <Number of trials> Controls number of events generated. Default is 10,000");
  puts(" -d <Database file name> Default is '/u/home/orsborn/vdcsim/DB/20030415/db_L.vdc.dat' ");
  puts(" -n <Noise> Simulated noise sigma value (>0). Default is 4.5");
  puts(" -e <Wire Efficiency> (In decimal form). Default is 1");
  puts(" -r <emission rate> Target's Emission Rate (in particles/ns). Default is 0.000002 (2 kHz)");
  puts (" -t <tdc time window> Length of time the TDCs can collect data (in ns). Default is 900");
  exit(1);
}

void getargs( int argc, char **argv, THaVDCSimConditions* s )
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
	  s->filename = opt;
	  opt = "?";
	  break;
	case 'f':
	  if(!*++opt){
	    if (argc-- < 1)
	      usage();
	    opt = *++argv;
	  }
	  s->textFile = opt;
	  opt = "?";
	  break;
	case 'a':
	  if(!*++opt){
	    if (argc-- < 1)
	      usage();
	    opt = *++argv;
	  }
	  s->numTrials = atoi(opt);
	  if ( s->numTrials < 0.0 )
	    usage();
	  opt = "?";
	  break;
	case 'd':
	  if(!*++opt){
	    if (argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->databaseFile = opt;
	  opt = "?";
	  break;
	case 'n':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->noiseSigma = atof(opt);
	  if( s->noiseSigma < 0.0 )
	    usage();
	  opt = "?";
	  break;
	case 'e':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->wireEff = atof(opt);
	  if( s->wireEff > 1.0)
	    usage();
	  opt = "?";
	  break;
	case 'r':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->emissionRate = atof(opt);
	  if( s->emissionRate < 0.0)
	    usage();
	  opt = "?";
	  break;
	case 't':
	  if(!*++opt){
	    if(argc-- <1)
	      usage();
	    opt = *++argv;
	  }
	  s->tdcTimeLimit = atof(opt);
	  if( s->tdcTimeLimit < 0.0)
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

int vdcsimgen( THaVDCSimConditions* s )
{
  // Create a new ROOT binary machine independent file.
  // This file is now becoming the current directory.
  TFile hfile(s->filename,"RECREATE","ROOT file for VDC simulation");

  // Define the height of the sense wires
  s->set(s->wireHeight, 0, s->cellHeight, s->planeSpacing, s->cellHeight + s->planeSpacing);
  s->set(s->wireUVOffset, 0,0, s->planeSpacing/sqrt(2.0), s->planeSpacing/sqrt(2.0));
  
  //calculate probability using Poisson distribution
  Float_t probability = 1.0 - exp(-s->emissionRate*s->tdcTimeLimit);

  // driftVelocity is in m/ns
  // s->set(s->driftVelocities, .0000504, .0000511, .0000509, .0000505);

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
  TH1F *hnumtracks = new TH1F ( "hnumtracks", "Number of Tracks Per Event", 5, 0, 5);

  THaVDCSimEvent *event = new THaVDCSimEvent;
  TTree *tree = new TTree("tree","VDC Track info");
  TBranch *eventBranch = tree->Branch("event", "THaVDCSimEvent", &event);

  //open text file to put track info into
  ofstream textFile(s->textFile, ios::out);
  if(!textFile.is_open())
    cout << "Error Opening Text File\n";

  //write important sim conditions to text file
  textFile << "******Simulation Conditions******\n"
	   << "Output File Name = " << s->filename << endl
	   << "Database File Read = " << s->databaseFile << endl
	   << "Number of Trials = " << s->numTrials << endl
	   << "Noise Sigma = " << s->noiseSigma << " ns\n"
	   << "Wire Efficiency = " << s->wireEff << endl
	   << "Emission Rate = " << s->emissionRate << " particle/ns\n"
	   << "TDC Time Window = " << s->tdcTimeLimit << " ns\n\n";

  //create 10000 simulated tracks
  for ( Int_t i=0; i< s->numTrials; i++) {

    //set track type, track number, and event number each time loop executes
    Int_t tracktype = 0, tracknum = 0;
    event->event_num = i + 1;
    textFile << "\nEvent: " << event->event_num << endl << "***************************************\n";

  newtrk:
    THaVDCSimTrack *track = new THaVDCSimTrack(tracktype,tracknum);

    // create randomized origin and momentum vectors for trigger and coincident tracks
    if(track->type == 0 || track->type == 1){
      track->origin.SetXYZ(gRandom->Rndm(1)*(s->x2-s->x1) + s->x1, gRandom->Gaus(s->ymean, s->ysigma), s->z0);
      track->momentum.SetXYZ(0,0,s->pmag0);
      track->momentum.SetTheta(gRandom->Gaus(s->pthetamean, s->pthetasigma));
      track->momentum.SetPhi(gRandom->Gaus(s->pphimean, s->pphisigma));
    }

    //write track info to text file
    textFile << "Track number = " << tracknum << ", type = " << tracktype << endl
	     << "Origin = ( " << track->origin.X() << ", " << track->origin.Y() << ", " 
	     << track->origin.Z() << " )" << endl
	     << "Momentum = ( " << track->momentum.X() << ", " << track->momentum.Y() <<", "
	     << track->momentum.Z() << " )\n";

    //Fill histogram with origin position
    horigin->Fill(track->X(), track->Y());

    //generate a time offset for the track
    if(track->type == 0)
      track->timeOffset = 0.0;
    else if(track->type == 1)
      track->timeOffset = gRandom->Rndm(1)*s->tdcTimeLimit; 
    //****Note******* 
    //Here we use a linear approximation of the exponential 
    //probability of s->timeOffset. 
    //Assuming low emission rates (< 10kHz) and time windows of ~100 ms, this is acceptable.


    //fill track information
    event->tracks.Add(track);

    //run through each plane (u1, v1, u2, v2)
    for (Int_t j=0; j < 4; j++) 
    {    
      textFile << "\nPlane: " << j << endl;
      // figuring out which wires are hit
      
      //calculate time it takes to hit wire plane and the position at which it hits
      Float_t travelTime = (s->wireHeight[j]-track->origin.Z())/track->momentum.Z();
      TVector3 position = track->origin + travelTime*track->momentum;
 
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
      Float_t noiseTimes[6];  //arrays to hold times when 5 wires hit (to calc. relative time)

      pivotWire = static_cast<Int_t>(x/s->cellWidth);
      wirehitFirst = static_cast<Int_t>((x - s->cellHeight/tanThetaPrime/2)/s->cellWidth);
      wirehitLast = static_cast<Int_t>((x + s->cellHeight/tanThetaPrime/2)/s->cellWidth);
      
      //write the hit wires to text file
      textFile << "Wires Hit = \t\t";
      for(int m = wirehitFirst; m <= wirehitLast; m++)
	textFile << m << "\t\t";
      textFile << endl << "Hit Times (ns) = \t";

      Int_t numwires = wirehitLast - wirehitFirst + 1;
      bool aWireFailed = false;

      //loop through each hit wire
      for (Int_t k=wirehitFirst; k<=wirehitLast; k++) 
      {
	//declare wirehit class
	THaVDCSimWireHit *hit = new THaVDCSimWireHit;

	hit->wirenum = k;

	// distance from crossing point to wire "k"
	Float_t d = x - (k+.5)* s->cellWidth;
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

	hit->rawTime = d0;

	textFile << hit->rawTime << "\t";

	//convert rawTime to tdctime
	hit->rawTDCtime = (timeOffsets[k+j*s->numWires] - s->tdcConvertFactor*(travelTime 
	  // Correction for the velocity of the track
	  // This correction is negligible, because of high momentum

	  // Ignoring this correction permits momentum to have any
	  // magnitude with no effect on the results
	  //+ d*sqrt(pow(tanThetaPrime,2)+1)/track->momentum.Mag()
	  
	  // Correction for ionization drift velocities
	  + hit->rawTime/s->driftVelocities[j] + track->timeOffset));

	// reject data that the noise filter would
	// This shouldn't affect our data at all
	/*
	if (d0 < -.5 * .013 ||
	    d0 > 1.5 * .013) {
	  delete hit;
	  continue;
	}
	*/
	
	hit->time = hit->rawTDCtime + s->tdcConvertFactor*(gRandom->Gaus(s->noiseMean, s->noiseSigma)); 
	  //time with additional noise to account for random walk nature of ions

	//if 4,5, or 6 wires hit, keep track of each time 
	if(numwires >= 4 && numwires <= 6)
	{
	  times[counter] = hit->rawTDCtime;
	  noiseTimes[counter] = hit->time;
	}
	
	counter++;  //increase counter

	//check to assure hit is within bounds	
	if (hit->wirenum < 0 || hit->wirenum > s->numWires - 1){
	  delete hit;
	  continue;
	}
 	//make sure hit time falls within time tdc is open
	if (hit->time < 0.0){
	  delete hit;
	  continue;
	}

	//simulate efficiency of wires.
	double workFail = gRandom->Rndm(1);
	if(workFail > s->wireEff){
	  delete hit;
	  hit->wireFail = true;
	  aWireFailed = true;
	  continue;
	}

	//fill histograms for v1 plane
	if (j == 1) 
	{
	  hdrift->Fill((hit->rawTDCtime));
	  hdriftNoise->Fill((hit->time));
	  hdrift2->Fill(wirehitLast-wirehitFirst+1, (hit->time));
	  hwire->Fill(k);
	}

	//fill wirehit information
	event->wirehits[j]->Add(hit);
	track->hits[j].Add(hit);

      } //closes loop going through each hit wire
      
      //write tdc times to text file
      textFile << endl << "Raw TDC Times = \t";
      for(int m = 0; m < numwires; m++)
	textFile << times[m] << "\t\t";
      textFile << endl << "Final TDC Times = \t";
      for(int m = 0; m < numwires; m++)
	textFile << noiseTimes[m] << "\t\t";
      textFile << endl;
      
      //fill number of wire histograms for each plane
      if(j == 0)      
	hnumwiresU1->Fill(wirehitLast - wirehitFirst + 1);
      else if(j == 1)
	hnumwiresV1->Fill(wirehitLast - wirehitFirst + 1);
      else if(j == 2)
	hnumwiresU2->Fill(wirehitLast - wirehitFirst + 1);
      else
	hnumwiresV2->Fill(wirehitLast - wirehitFirst +1);

      // calculate relative time and store in histograms for 4, 5, and 6 hits
      // use 10^38 if a wire failed during the hit (since resolution would be completely lost!)
      if(aWireFailed){
	if(numwires == 4){
	  hdeltaTNoise4->Fill(100000000000000000000000000000000000000.0);
	  hdeltaT4->Fill(100000000000000000000000000000000000000.0);
	}
	else if(numwires == 5){
	  hdeltaTNoise5->Fill(100000000000000000000000000000000000000.0);
	  hdeltaT5->Fill(100000000000000000000000000000000000000.0);
	}
        else if(numwires == 6){
	  hdeltaTNoise6->Fill(100000000000000000000000000000000000000.0);
	  hdeltaT6->Fill(100000000000000000000000000000000000000.0);
	}
      }
       else{
	if(numwires == 4)
	  {
	    hdeltaTNoise4->Fill((noiseTimes[0] - noiseTimes[1]) - (noiseTimes[3] - noiseTimes[2]));
	    hdeltaT4->Fill((times[0] - times[1]) - (times[3] - times[2]));
	  }
	else if(numwires == 5)
	  {
	    hdeltaTNoise5->Fill((noiseTimes[0] - noiseTimes[1]) - (noiseTimes[4] - noiseTimes[3]));
	    hdeltaT5->Fill((times[0] - times[1]) - (times[4] - times[3]));
	  }
	else if(numwires == 6)
	  {
	    hdeltaTNoise6->Fill((noiseTimes[0] - noiseTimes[1]) - (noiseTimes[5] - noiseTimes[4]));
	    hdeltaT6->Fill((times[0] - times[1]) - (times[5] - times[4]));
	  }
       }

    }//closes loop going through each plane

    //if target track , decide if there will be coincident track
    if(track->type == 0){
       double secondTrack = gRandom->Rndm(1);
       if( secondTrack <= probability ){
	tracktype = 1;
	tracknum++;
	goto newtrk;  //go back to where new tracks are created, still within the same event

       }
    }

    //*******all tracks for the single event have now been generated*******
   
    //fill histogram of number of tracks per event
    hnumtracks->Fill(tracknum + 1);
    
    //fill the tree with event information
    eventBranch->Fill();
    
    //clean up. This clears wirehits Tlists and the track TList
    event->Clear();

   } //closes loop going through all the trials

  //close text file
  textFile.close();
  if(textFile.is_open())
    cout << "Error Closing Text File\n";

  // Save objects in this file
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
  hnumtracks->Write();
 
  // Close the file.
  hfile.Close();

  return 0;
}
