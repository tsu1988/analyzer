#ifndef ROOT_THaVDCPlane
#define ROOT_THaVDCPlane

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPlane                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "THaVDCWire.h"
#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include <cassert>
#include <vector>

class THaEvData;
class THaVDCTimeToDistConv;
class THaTriggerTime;
class THaVDC;

typedef std::vector<THaVDCWire>    VWires_t;
typedef std::vector<THaVDCHit>     VHits_t;
typedef std::vector<THaVDCCluster> VClusters_t;

class THaVDCPlane : public THaSubDetector {

public:

  THaVDCPlane( const char* name="", const char* description="",
	       THaDetectorBase* parent = NULL );
  virtual ~THaVDCPlane();

  virtual void    Clear( Option_t* opt="" );
  virtual Int_t   Decode( const THaEvData& ); // Raw data -> hits
  virtual Int_t   FindClusters();             // Hits -> clusters
  virtual Int_t   FitTracks();                // Clusters -> tracks

  //Get and Set functions
  Int_t              GetNClusters()      const { return fClusters.size(); }
  const VClusters_t& GetClusters()       const { return fClusters; }
  THaVDCCluster*     GetCluster(Int_t i)
  {
    assert( i>=0 && i<GetNClusters() );
    return &fClusters[i];
  }

  Int_t              GetNWires()         const { return fWires.size(); }
  const VWires_t&    GetWires()          const { return fWires; }
  THaVDCWire*        GetWire(Int_t i)
  {
    assert( i>=0 && i<GetNWires() );
    return &fWires[i];
  }

  Int_t              GetNHits()          const { return fHits.size(); }
  const VHits_t&     GetHits()           const { return fHits; }
  THaVDCHit*         GetHit(Int_t i)
  {
    assert( i>=0 && i<GetNHits() );
    return &fHits[i];
  }
  UInt_t         GetNWiresHit()      const { return fNWiresHit; }

  Double_t       GetZ()              const { return fZ; }
  Double_t       GetWBeg()           const { return fWBeg; }
  Double_t       GetWSpac()          const { return fWSpac; }
  Double_t       GetWAngle()         const { return fWAngle; }
  Double_t       GetCosAngle()       const { return fCosAngle; }
  Double_t       GetSinAngle()       const { return fSinAngle; }
  Double_t       GetTDCRes()         const { return fTDCRes; }
  Double_t       GetDriftVel()       const { return fDriftVel; }
  Double_t       GetMinTime()        const { return fMinTime; }
  Double_t       GetMaxTime()        const { return fMaxTime; }
  Double_t       GetT0Resolution()   const { return fT0Resolution; }

//   Double_t GetT0() const { return fT0; }
//   Int_t GetNumBins() const { return fNumBins; }
//   Float_t *GetTable() const { return fTable; }

protected:

  VWires_t    fWires;        // Wires
  VHits_t     fHits;         // Fired wires
  VClusters_t fClusters;     // Clusters

  UInt_t      fNHits;        // Total number of hits (including multihits)
  UInt_t      fNWiresHit;    // Number of wires with one or more hits

  // Parameters, read from database
  UInt_t fMinClustSize;      // Minimum number of wires needed for a cluster
  UInt_t fMaxClustSpan;      // Maximum size of cluster in wire spacings
  UInt_t fNMaxGap;           // Max gap in wire numbers in a cluster
  Int_t  fMinTime, fMaxTime; // Min and Max limits of TDC times for clusters
  Int_t  fFlags;             // Analysis control flags

  Double_t fClustChi2Cut;    // Max chi2/dof allowed for keeping clusters

  //  Double_t fMinTdiff, fMaxTdiff;  // Min and Max limits of times between wires in cluster

  Double_t fZ;            // Z coordinate of plane in U1 coord sys (m)
  Double_t fWBeg;         // Position of 1-st wire in E-arm coord sys (m)
  Double_t fWSpac;        // Wire spacing and direction (m)
  Double_t fWAngle;       // Angle (rad) between dispersive direction (x) and
                          // normal to wires in dir of increasing wire position
  Double_t fSinAngle;     // sine of fWAngle, for efficiency
  Double_t fCosAngle;     // cosine of fWAngle, for efficiency
  Double_t fTDCRes;       // TDC Resolution ( s / channel)
  Double_t fDriftVel;     // Drift velocity in the wire plane (m/s)

  Double_t fT0Resolution; // (Average) resolution of cluster time offset fit

  // Lookup table parameters
//   Double_t fT0;     // calculated zero time
//   Int_t fNumBins;   // size of lookup table
//   Float_t *fTable;  // time-to-distance lookup table

  THaVDCTimeToDistConv* fTTDConv;  // Time-to-distance converter for this plane's wires

  THaVDC* fVDC;           // VDC detector to which this plane belongs

  THaTriggerTime* fglTrg; //! time-offset global variable. Needed at the decode stage

  virtual void  MakePrefix();
  virtual Int_t ReadDatabase( const TDatime& date );
  virtual Int_t DefineVariables( EMode mode = kDefine );

  THaVDCCluster* AddCluster( const THaVDCCluster& newCluster );

  ClassDef(THaVDCPlane,0)             // VDCPlane class
};

//////////////////////////////////////////////////////////////////////////////

#endif
