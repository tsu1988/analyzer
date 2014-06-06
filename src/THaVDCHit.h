#ifndef ROOT_THaVDCHit
#define ROOT_THaVDCHit

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCHit                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCWire.h"
#include <cstdio>
#include <cassert>
#include <functional>
#include <list>

class THaVDCCluster;

namespace VDC {
  typedef std::list<THaVDCCluster*> LClust_t;
  extern const Double_t kBig;
}
using namespace VDC;

class THaVDCHit : public TObject {

public:
  THaVDCHit( THaVDCWire* wire=0, Int_t rawtime=0, Double_t time=0.0 )
    : fWire(wire), fRawTime(rawtime), fTime(time), fDist(kBig), fdDist(1.0),
      ftrDist(kBig), fTrkNum(0), fClsNum(-1) {}
  virtual ~THaVDCHit() {}

  virtual Double_t ConvertTimeToDist( Double_t slope, Double_t t0 = 0 );
  Int_t  Compare ( const TObject* obj ) const;
  Bool_t IsSortable () const { return kTRUE; }

  // Get and Set Functions
  THaVDCWire* GetWire() const { return fWire; }
  Int_t    GetWireNum() const { return fWire->GetNum(); }
  Int_t    GetRawTime() const { return fRawTime; }
  Double_t GetTime()    const { return fTime; }
  Double_t GetDist()    const { return fDist; }
  Double_t GetPos()     const { return fWire->GetPos(); } //Position of hit wire
  Double_t GetdDist()   const { return fdDist; }
  Int_t    GetTrkNum()  const { return fTrkNum; }
  Int_t    GetClsNum()  const { return fClsNum; }

  void     SetWire(THaVDCWire * wire) { fWire = wire; }
  void     SetRawTime(Int_t time)     { fRawTime = time; }
  void     SetTime(Double_t time)     { fTime = time; }
  void     SetDist(Double_t dist)     { fDist = dist; }
  void     SetdDist(Double_t ddist)   { fdDist = ddist; }
  void     SetFitDist(Double_t dist)  { ftrDist = dist; }
  void     SetLocalFitDist(Double_t dist)  { fltrDist = dist; }
  void     SetTrkNum(Int_t num)       { fTrkNum = num; }
  void     SetClsNum(Int_t num)       { fClsNum = num; }

  // Support for cluster list
  void             AddCluster( THaVDCCluster* cl ) { fClusters.push_back(cl); }
  LClust_t&        GetClusters()                   { return fClusters; }
  const LClust_t&  GetClusters() const             { return fClusters; }

  // Functors for ordering pointers to hits
  struct ByPosThenTime :
    public std::binary_function< const THaVDCHit*, const THaVDCHit*, bool >
  {
    bool operator() ( const THaVDCHit* a, const THaVDCHit* b ) const
    {
      assert( a && b );
      if( a->GetWireNum() != b->GetWireNum() )  // Same wire num <=> same pos
	return ( a->GetPos() < b->GetPos() );
      return ( a->GetTime() < b->GetTime() );
    }
  };
  struct ByTime :
    public std::binary_function< THaVDCHit*, THaVDCHit*, bool >
  {
    bool operator() ( const THaVDCHit* a, const THaVDCHit* b ) const
    {
      assert( a && b );
      return ( a->GetTime() < b->GetTime() );
    }
  };

  // Default sorting of hits: ByPosThenTime
  bool operator<( const THaVDCHit& rhs ) const {
    return ( Compare(&rhs) == -1 );
  }

protected:
  THaVDCWire* fWire;     // Wire on which the hit occurred
  Int_t       fRawTime;  // TDC value (channels)
  Double_t    fTime;     // Raw drift time, corrected for trigger time (s)
  Double_t    fDist;     // (Perpendicular) Drift Distance
  Double_t    fdDist;    // uncertainty in fDist (for chi2 calc)
  Double_t    ftrDist;   // (Perpendicular) distance from the global track (m)
  Double_t    fltrDist;  // (Perpendicular) distance from the local track (m)
  Int_t       fTrkNum;   // Number of the track using this hit (0 = unused)
  Int_t       fClsNum;   // Number of the cluster using this hit (-1 = unused)
  LClust_t    fClusters; // All clusters using this hit

  ClassDef(THaVDCHit,2)             // VDCHit class
};

///////////////////////////////////////////////////////////////////////////////
#endif
