#ifndef ROOT_THaVDCHit
#define ROOT_THaVDCHit

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCHit                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "THaVDCWire.h"

class THaVDCHit : public TObject {

public:
  THaVDCHit( THaVDCWire* wire=NULL, Int_t rawtime=0, Double_t time=0.0 ) : 
    fWire(wire), fRawTime(rawtime), fFlag(0), fTime(time), fDist(0.0), 
    fFitDist(0.0) {}
  virtual ~THaVDCHit() {}

  virtual Double_t ConvertTimeToDist(Double_t slope);
  Int_t  Compare ( const TObject* obj ) const;
  Bool_t IsSortable () const { return kTRUE; }
  
  // Get and Set Functions
  THaVDCWire* GetWire() const { return fWire; }
  Int_t    GetWireNum() const { return fWire->GetNum(); }
  Int_t    GetRawTime() const { return fRawTime; }
  Int_t    GetFlag()    const { return fFlag; }
  Double_t GetTime()    const { return fTime; }
  Double_t GetDist()    const { return fDist; }
  Double_t GetFitDist() const { return fFitDist; }
  Double_t GetPos()     const { return fWire->GetPos(); } //Position of hit wire


  void     SetWire(THaVDCWire* wire)  { fWire = wire; }
  void     SetRawTime(Int_t time)     { fRawTime = time; }
  void     SetTime(Double_t time)     { fTime = time; }
  void     SetDist(Double_t dist)     { fDist = dist; }
  void     SetFitDist(Double_t dist)  { fFitDist = dist; } 
  void     SetFlag(Int_t n)           { fFlag = n; }

protected:
  THaVDCWire* fWire;     // Wire on which the hit occurred
  Int_t       fRawTime;  // TDC value (channels)
  Int_t       fFlag;     // Indicator how this hit was used (e.g. track number)
  Double_t    fTime;     // Time corrected for time offset of wire (s)
  Double_t    fDist;     // Raw drift distance from T2D conversion
  Double_t    fFitDist;  // Drift distance from cluster fit (valid if fUsed > 0)

private:
  THaVDCHit( const THaVDCHit& );
  THaVDCHit& operator=( const THaVDCHit& );
  
  ClassDef(THaVDCHit,1)             // VDCHit class
};

////////////////////////////////////////////////////////////////////////////////

#endif
