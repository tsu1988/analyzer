#ifndef HALLA_VDCeff
#define HALLA_VDCeff

//////////////////////////////////////////////////////////////////////////
//
// VDCeff - VDC efficiency calculation
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"

class VDCeff : public THaPhysicsModule {
public:
  VDCeff( const char* name, const char* description );
  virtual ~VDCeff();
  
  virtual void    Clear( Option_t* opt="" );
  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

protected:

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

