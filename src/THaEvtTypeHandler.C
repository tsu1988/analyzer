////////////////////////////////////////////////////////////////////
//
//   THaEvtTypeHandler
//   handles events of a particular type
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include <iostream>
#include <cstring>
#include <sstream>

using namespace std;

//_____________________________________________________________________________
THaEvtTypeHandler::THaEvtTypeHandler(const char* name, const char* description)
  : THaAnalysisObject(name, description), fDebugFile(0)
{
}

//_____________________________________________________________________________
THaEvtTypeHandler::~THaEvtTypeHandler()
{
  if (fDebugFile) {
    //    fDebugFile->close();
    delete fDebugFile;
  }
}

//_____________________________________________________________________________
void THaEvtTypeHandler::AddEvtType(Int_t evtype)
{
  eventtypes.push_back(evtype);
}
  
//_____________________________________________________________________________
void THaEvtTypeHandler::SetEvtType(Int_t evtype)
{
  eventtypes.clear();
  AddEvtType(evtype);
}

//_____________________________________________________________________________
Int_t THaEvtTypeHandler::GetEvtType(Int_t i) const
{
  if( GetNumTypes() == 0 || i>=GetNumTypes() ) return -1;
  return eventtypes[i];
}

//_____________________________________________________________________________
void THaEvtTypeHandler::Print( Option_t* ) const
{
  cout << "Hello !  THaEvtTypeHandler name =  "<<GetName()<<endl;
  cout << "    description "<<GetTitle()<<endl;
  cout << "    event types handled are "<<endl;
  for (UInt_t i=0; i < eventtypes.size(); i++) {
    cout << "    event type "<<eventtypes[i]<<endl;
  }
  cout << "----------------- good bye ----------------- "<<endl;
}

//_____________________________________________________________________________
void THaEvtTypeHandler::EvDump(THaEvData *evdata) const
{
  if( evdata && fDebugFile )
    evdata->dump(*fDebugFile);
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaEvtTypeHandler::Init(const TDatime&)
{
  fStatus = kOK;
  return kOK;
}

//_____________________________________________________________________________
void THaEvtTypeHandler::SetDebugFile(const char *filename)
{
  if( filename && *filename ) {
    delete fDebugFile;
    fDebugFile = new ofstream(filename);
  } else {
    Error( "THaEvtTypeHandler::SetDebugFile", "Must specify a file name" );
  }
}

//_____________________________________________________________________________
Bool_t THaEvtTypeHandler::IsMyEvent(Int_t evnum) const
{
  for (UInt_t i=0; i < eventtypes.size(); i++) {
    if (evnum == eventtypes[i]) return kTRUE;
  }

  return kFALSE;
}

//_____________________________________________________________________________
void THaEvtTypeHandler::MakePrefix()
{
  THaAnalysisObject::MakePrefix( NULL );
}

ClassImp(THaEvtTypeHandler)
