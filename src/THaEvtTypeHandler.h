#ifndef THaEvtTypeHandler_
#define THaEvtTypeHandler_

/////////////////////////////////////////////////////////////////////
//
//   THaEvtTypeHandler
//   Base class to handle different types of events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include <vector>
#include <fstream>

class THaEvData;

class THaEvtTypeHandler : public THaAnalysisObject {

public:

  THaEvtTypeHandler(const char* name, const char* description);
  virtual ~THaEvtTypeHandler();

  virtual Int_t   Analyze(THaEvData *evdata) = 0;
  virtual EStatus Init( const TDatime& run_time );
  virtual void    Print( Option_t* opt="" ) const;
  virtual void    EvDump( THaEvData* evdata ) const;
  virtual Bool_t  IsMyEvent(Int_t evnum) const;
  virtual void    SetDebugFile(std::ostream *file) { if (file!=0) fDebugFile=file; };
  virtual void    SetDebugFile(const char *filename);
  virtual void    AddEvtType(Int_t evtype);
  virtual void    SetEvtType(Int_t evtype);

          Int_t   GetNumTypes() const { return eventtypes.size(); };
          Int_t   GetEvtType( Int_t i=0 ) const;

  const std::vector<Int_t>&
                  GetEvtTypes() const { return eventtypes; };

          void    EvPrint() const { Print(); }

protected:

  std::vector<Int_t> eventtypes;
  virtual void MakePrefix();
  std::ostream *fDebugFile;

private:

  THaEvtTypeHandler(const THaEvtTypeHandler &fh);
  THaEvtTypeHandler& operator=(const THaEvtTypeHandler &fh);

  ClassDef(THaEvtTypeHandler,0)  // Event type handler

};

#endif
