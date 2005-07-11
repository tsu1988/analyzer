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
  THaVDCSimConditions() //set defaults for conditions
    : filename("vdctracks.root"), textFile("trackInfo.data()"), numTrials(10000), 
    databaseFile("/u/home/orsborn/vdcsim/DB/20030415/db_L.vdc.dat"),
    numWires(368), noiseSigma(4.5), noiseMean(0.0), wireEff(1.0), tdcTimeLimit(900.0), 
    emissionRate(0.000002), probWireNoise(0.0), x1(-0.8), x2(0.9),ymean(0.0), ysigma(0.01), z0(-1.0), 
    pthetamean(TMath::Pi()/4.0), pthetasigma(atan(1.1268) - TMath::Pi()/4.0), 
    pphimean(0.0), pphisigma(atan(0.01846)), pmag0(1.0), tdcConvertFactor(2.0), cellWidth(0.00424), 
    cellHeight(0.026), planeSpacing(0.335)  {}
  
  TString filename;           //name of output file
  TString textFile;           //name of output text file (holds track information)
  int numTrials;              //number of events generated in monte carlo
  Double_t wireHeight[4];      // Height of each wire in Z coord
  Double_t wireUVOffset[4];    // Wire Offset in the coord of its plane
  Double_t driftVelocities[4]; // Drift velocity in Z coord
  TString Prefixes[4];        //array of prefixes to use in reading database
  TString databaseFile;       //name of database file to read
  int numWires;               //number of wires in each chamber
  double noiseSigma;          //sigma value of noise distribution
  double noiseMean;           //mean of noise distribution 
  double wireEff;             //wire efficiency (decimal form)
  double tdcTimeLimit;        //window of time the tdc reads (in ns)
  double emissionRate;        //rate particles are emitted from target (in particles/ns)
  double probWireNoise;     //probability of random chamber wires firing
  const Double_t x1;      
  const Double_t x2;           //x coordinate limits for origin of tracks
  const Double_t ymean;
  const Double_t ysigma;       //mean and sigma value for origin  y coord. distribution
  const Double_t z0;           //z coordinate of track origin
  const Double_t pthetamean;
  const Double_t pthetasigma;  //mean and sigma for momentum theta distribution 
  const Double_t pphimean;
  const Double_t pphisigma;    //mean and sigma of momentum phi distribution
  const Double_t pmag0;        //magnitude of momentum
  Double_t tdcConvertFactor;   //width of single time bin (in ns), used to convert from time to tdc values
  const Double_t cellWidth;    //width of cell (i.e. horizontal spacing between sense wires)
  const Double_t cellHeight;   //height of cell (i.e. vertical spacing between u and v wire planes)
  const Double_t planeSpacing; //vertical separation between the two planes of sense wires

  void set(Double_t *something,
	   Double_t a, Double_t b, Double_t c, Double_t d);

  Int_t ReadDatabase(int j, Double_t* timeOffsets, const int numWires);
 
 ClassDef (THaVDCSimConditions, 4) // Simulation Conditions
};

class THaVDCSimWireHit : public TObject {
public:
  THaVDCSimWireHit()
    : wirenum(0), type(0), rawTime(0), rawTDCtime(0), time(0), distance(0), wireFail(false) {}
  Int_t wirenum;      // Wire number
  Int_t type;         // flag for tyep of hit. actual = 0, noise = 1
  Double_t rawTime;    // Time of wire hit in nanoseconds
  Int_t rawTDCtime; //tdc time w/out noise
  Int_t time;       //tdctime w/additional noise
  Double_t distance;   //drift distance 
  bool wireFail;      //set to true when the wire fails

  ClassDef (THaVDCSimWireHit, 3) // Simple Wirehit class
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

  ClassDef (THaVDCSimEvent, 2) // Simple simulated track class
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
  Double_t ray[5];  //position on U1 plane in transport coordinates
  Double_t tanThetaPrime;    //tangent of angle of track

  Int_t type;   //type of track. 0 = trigger, 1 = coincident, 2 = delta ray, 3 = cosmic ray
  Int_t layer;  //layer of material track originated from. 0 = target, 1 = vacuum window, 2 = vdc frame etc.
  Int_t track_num;  //track index
  Double_t timeOffset; //time offset of the track (0 if trigger, random otherwise)

  TList hits[4]; // Hits of this track only in each wire plane

  virtual void Clear( const Option_t* opt="" );
  virtual void Print( const Option_t* opt="" );
  
  ClassDef (THaVDCSimTrack, 2) // Simluated VDC track
};

#endif
