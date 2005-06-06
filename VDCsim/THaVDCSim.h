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
  Float_t timeOffset; // Offset of the TDCs

  //Gonna need at some point a time offset for each WIRE

  void set(Float_t *something,
	   Float_t a, Float_t b, Float_t c, Float_t d);

  ClassDef (THaVDCSimConditions, 1) // Simulation Conditions
};

class THaVDCSimWireHit : public TObject {
public:
  THaVDCSimWireHit()
    : wirenum(0), time(0) {}
  Int_t wirenum; // Wire number
  Float_t time; // Time of wire hit

  ClassDef (THaVDCSimWireHit, 1) // Simple Wirehit class
};

class THaVDCSimEvent : public TObject {
public:
  THaVDCSimEvent();
  ~THaVDCSimEvent();

  // Administrative data
  Int_t event_num;

  // "Real" data
  TVector3 origin; // Origin of track
  TVector3 momentum; // Momentum of track
  Double_t X() {return origin.X();}
  Double_t Y() {return origin.Y();}
  Double_t Theta() {return tan(momentum.Theta());}

  // "Simulated" data
  TList *wirehits[4]; //list of hits for each set of wires (u1,v1,u2,v2)

  virtual void Clear( const Option_t* opt="" );

  ClassDef (THaVDCSimEvent, 1) // Simple simulated track class
};

#endif
