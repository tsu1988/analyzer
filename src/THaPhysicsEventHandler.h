#ifndef ROOT_THaPhysicsEventHandler
#define ROOT_THaPhysicsEventHandler

//////////////////////////////////////////////////////////////////////////
//
// THaPhysicsEventHandler
//
//////////////////////////////////////////////////////////////////////////

#include "THaEventTypeHandler.h"

class THaPhysicsEventHandler : public THaEventTypeHandler {
  
public:
  THaPhysicsEventHandler();
  virtual ~THaPhysicsEventHandler();
  
  virtual void  Clear( Option_t* opt="" );
  virtual Int_t Init( const TDatime& date, const TList* modules );
  virtual Int_t InitLevel2();
  virtual Int_t Begin();
  virtual Int_t End();
  virtual Int_t Analyze( const THaEvData& evdata, Int_t status ) = 0;
  virtual void  Print( Option_t* opt="" ) const;

  TList*  GetApps()    const { return fApps; }
  TList*  GetPhysics() const { return fPhysics; }

protected:

  Bool_t fFirst;
  Counter fNanalyzed;
  TList* fApps;
  TList* fPhysics;

  // Cut/histos to evaluate at each stage
  Stage* fDecodeTests;
  Stage* fCoarseTrackTests;
  Stage* fCoarseReconTests;
  Stage* fTrackingTests;
  Stage* fReconstructTests;
  Stage* fPhysicsTests;


  ClassDef(THaPhysicsEventHandler,0)
};

#endif
