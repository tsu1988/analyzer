#ifndef HALLA_VDCeff
#define HALLA_VDCeff

//////////////////////////////////////////////////////////////////////////
//
// VDCeff - VDC efficiency calculation
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include <vector>

class THaVar;
class TH1F;

class VDCeff : public THaPhysicsModule {
public:
  VDCeff( const char* name, const char* description );
  virtual ~VDCeff();
  
  virtual void    Clear( Option_t* opt="" );
  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

protected:

  typedef std::vector<Long64_t> Vcnt_t;
  typedef const THaVar CVar_t;

  struct VDCvar_t {
    TString  name;
    TString  histname;
    CVar_t*  var;
    Int_t    nwire;
    Vcnt_t   ncnt;
    Vcnt_t   nhit;
    TH1F*    hist_nhit;
    TH1F*    hist_eff;
    VDCvar_t( const char* nm, const char* hn, Int_t nw )
      : name(nm), histname(hn), var(0), nwire(nw), hist_nhit(0), hist_eff(0) {}
    ~VDCvar_t();
  };

  std::vector<VDCvar_t>  fVDCvar;
  std::vector<Short_t>   fWire;
  std::vector<bool>      fHitWire;

  Int_t     fCycle;
  Int_t     fMaxNwire;
  Double_t  fMaxOcc;
  Long64_t  fNevt;

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(VDCeff,0)   // VDC hit efficiency physics module
};

#endif
//TODO
// class VDCeff : public THaPhysicsModule {
// public:

//   virtual Int_t   End(THaRunBase* r=0);
//   virtual void    WriteHist(); 

//   Int_t  cnt1;
//   void VdcEff();

//   static Int_t fgVdcEffFirst; //If >0, initialize VdcEff() on next call
// };

