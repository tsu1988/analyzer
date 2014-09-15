#ifndef Podd_Apparatus
#define Podd_Apparatus

//////////////////////////////////////////////////////////////////////////
//
// Apparatus
//
//////////////////////////////////////////////////////////////////////////

#include "AnalysisObject.h"

class THaEvData;
class THaRunBase;
class THaEvData;
class TList;

namespace Podd {

class Detector;

class Apparatus : public AnalysisObject {
  
public:
  virtual ~Apparatus();
  
  virtual Int_t        AddDetector( Detector* det );
  virtual Int_t        Begin( THaRunBase* r=0 );
  virtual void         Clear( Option_t* opt="" );
  virtual Int_t        Decode( const THaEvData& );
  virtual Int_t        End( THaRunBase* r=0 );
          Int_t        GetNumDets() const;
  virtual Detector*    GetDetector( const char* name );
  const   TList*       GetDetectors() { return fDetectors; } // for inspection

  virtual EStatus      Init( const TDatime& run_time );
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        CoarseReconstruct() { return 0; }
  virtual Int_t        Reconstruct() = 0;
  virtual void         SetDebugAll( Int_t level );

protected:
  TList*         fDetectors;    // List of all detectors for this apparatus

  Apparatus( const char* name, const char* description );
  Apparatus( );

  virtual void MakePrefix();

  ClassDef(Apparatus,1)   //A generic apparatus (collection of detectors)
};

} // end namespace Podd

// Backwards-compatibility
#define THaApparatus Podd::Apparatus
#ifndef THaDetector
# define THaDetector Podd::Detector
#endif

#endif

