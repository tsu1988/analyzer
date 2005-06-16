// THaVDCSim.h
//
// These are all data classes intended for storing results
// from randomly generated and digitized "simulated" data.
//
// author Ken Rossato (rossato@jlab.org)
//

#ifndef THaVDCSim_
#define THaVDCSim_

#include "TList.h"
#include "TVector3.h"

class THaVDCSimConditions : public TObject {
 public:
  THaVDCSimConditions() {}

  Float_t wireHeight[4]; // Height of each wire in Z coord
  Float_t wireUVOffset[4]; // Wire Offset in the coord of its plane
  Float_t driftVelocities[4]; // Drift velocity in Z coord
  TString Prefixes[4];  //array of prefixes to use
  int numWires;   //number of wires in each chamber
  double deltaTsigma;  //sigma value of noise distribution 
  Float_t tdcTimeLimit;  //window of time the tdc reads (in ns)
  Float_t emissionRate;  //rate particles are emitted from target (in particles/ns)

  void set(Float_t *something,
	   Float_t a, Float_t b, Float_t c, Float_t d);

  Int_t ReadDatabase(int j, float* timeOffsets, const int numWires);
 
 ClassDef (THaVDCSimConditions, 1) // Simulation Conditions
};

class THaVDCSimWireHit : public TObject {
public:
  THaVDCSimWireHit()
    : wirenum(0), time(0), timeN(0) {}
  Int_t wirenum; // Wire number
  Float_t time; // Time of wire hit
  Float_t timeN; //time of wire hit with additional noise
  Float_t coinTimeOffset; //time offset of the coincident track

  ClassDef (THaVDCSimWireHit, 2) // Simple Wirehit class
};

class THaVDCSimEvent : public TObject {
public:
  THaVDCSimEvent();
  virtual ~THaVDCSimEvent();

  // Administrative data
  Int_t event_num;

  // "Simulated" data
  TList *wirehits[4]; //list of hits for each set of wires (u1,v1,u2,v2)
  TList tracks;  //list of tracks for each plane

  virtual void Clear( const Option_t* opt="" );

  ClassDef (THaVDCSimEvent, 1) // Simple simulated track class
};

class THaVDCSimTrack : public TObject {
 public: 
  THaVDCSimTrack(Int_t type = 0, Int_t num = 0)
    : type(type), layer(0), track_num(num) {}

  TVector3 origin; // Origin of track
  TVector3 momentum; // Momentum of track
  Double_t X() {return origin.X();}
  Double_t Y() {return origin.Y();}
  Double_t Theta() {return tan(momentum.Theta());}

  Int_t type;   //type of track. 0 = trigger, 1 = coincident, 2 = delta ray, 3 = cosmic ray
  Int_t layer;  //layer of material track originated from. 0 = target, 1 = vacuum window, 2 = vdc frame etc.
  Int_t track_num;  //track index

  TList hits[4]; // Hits of this track only in each wire plane

  virtual void Clear( const Option_t* opt="" );
  virtual void Print( const Option_t* opt="" );
  
  ClassDef (THaVDCSimTrack, 1) // Simluated VDC track
};

#endif
