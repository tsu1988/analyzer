#ifndef ROOT_THaEventTypeHandler
#define ROOT_THaEventTypeHandler

//////////////////////////////////////////////////////////////////////////
//
// THaEventTypeHandler
//
//////////////////////////////////////////////////////////////////////////

// supports:
//  analysis stages
//  counters


#include "THaAnalyzer.h"
#include "TString.h"

class THaEvData;
class TDatime;
class TList;

class THaEventTypeHandler : public TObject {
  
public:
  THaEventTypeHandler();
  virtual ~THaEventTypeHandler();
  
  virtual void   Clear( Option_t* opt="" );
  virtual void   Close();
  virtual Int_t  Init( const TDatime& date, const TList* modules );
  virtual Int_t  InitLevel2();
  virtual Int_t  Begin();
  virtual Int_t  End();
  // TODO: really pass evdata?
  virtual Int_t  Analyze( const THaEvData& evdata, Int_t status ) = 0;
  virtual void   Print( Option_t* opt="" ) const;

  virtual Bool_t IsSortable() const { return kTRUE; }
  virtual Bool_t CanCoexistWith( THaEventTypeHandler* ) const { return kTRUE; }
  virtual Int_t  Compare( const TObject* ) const;

  void           SetSortPos( Int_t i );

protected:

  typedef THaAnalyzer::Counter Counter;
  typedef THaAnalyzer::Stage   Stage;

  Counter   fCount;             // Call counter
  Int_t     fSortPos;           // Processing order ordinal (smallest first)

  // Functions to access Analyzer status 
  // NB: with the current design, there is never more than one Analyzer
  UInt_t& GetNev() { return THaAnalyzer::fgAnalyzer->fNev; }
  Int_t   GetVerbose() const { return THaAnalyzer::fgAnalyzer->fVerbose; }
  THaBenchmark* GetBench() { return THaAnalyzer::fgAnalyzer->fBench; }
  THaRunBase* GetRun() { return THaAnalyzer::fgAnalyzer->fRun; }
  THaEvent*   GetEvent() { return THaAnalyzer::fgAnalyzer->fEvent; }
  THaOutput*  GetOutput() { return THaAnalyzer::fgAnalyzer->fOutput; }
  THaCutList* GetCuts() { return THaAnalyzer::fgAnalyzer->fCuts; }
  
  ClassDef(THaEventTypeHandler,1) // ABC for a raw data event type handler
};

#endif
