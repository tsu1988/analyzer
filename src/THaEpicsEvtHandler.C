////////////////////////////////////////////////////////////////////
//
//   THaEpicsEvtHandler
//
//   Event handler for Hall A EPICS data events
//   R. Michaels,  April 2015
//
//   This class does the following
//      For a particular set of event types (here, event type 131)
//      use the THaEpics class to decode the data.  This class interacts
//      with THaOutput much the same way the old (now obsolete) THaCodaDecoder
//      did, providing EPICS data to the output.
//
//   At the moment, I foresee this as a member of THaAnalyzer.
//   To use as a plugin with your own modifications, you can do this:
//       gHaEvtHandlers->Add (new THaEpicsEvtHandler("epics","HA EPICS event type 131"));
//
/////////////////////////////////////////////////////////////////////

#include "THaEpicsEvtHandler.h"
#include "THaEvData.h"
#include "THaEpics.h"
#include "TNamed.h"
#include "TMath.h"
#include <cstring>

using namespace std;
using namespace Decoder;

static const size_t fgInitMaxData = 20000;

THaEpicsEvtHandler::THaEpicsEvtHandler(const char *name, const char* description)
  : THaEvtTypeHandler(name,description), fBufsize(fgInitMaxData)
{
  fEpics = new Decoder::THaEpics();
  fEvtBuffer = new UInt_t[fBufsize];
}

THaEpicsEvtHandler::~THaEpicsEvtHandler()
{
  delete [] fEvtBuffer;
  delete fEpics;
}

Int_t THaEpicsEvtHandler::End( THaRunBase* )
{
  return 0;
}

Bool_t THaEpicsEvtHandler::IsLoaded(const char* tag) const {
  if ( !fEpics ) return kFALSE;
  return fEpics->IsLoaded(tag);
}

Double_t THaEpicsEvtHandler::GetData(const char* tag, Int_t event) const {
  assert( IsLoaded(tag) ); // Should never ask for non-existent data
  if ( !fEpics ) return 0;
  return fEpics->GetData(tag, event);
}

Double_t THaEpicsEvtHandler::GetTime(const char* tag, Int_t event) const {
  assert( IsLoaded(tag) );
  if ( !fEpics ) return 0;
  return fEpics->GetTimeStamp(tag, event);
}

TString THaEpicsEvtHandler::GetString(const char* tag, Int_t event) const {
  assert( IsLoaded(tag) );
  if ( !fEpics ) return TString("nothing");
  return TString(fEpics->GetString(tag, event).c_str());
}

Int_t THaEpicsEvtHandler::Analyze(THaEvData *evdata)
{

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  size_t evlen = static_cast<size_t>(evdata->GetEvLength());
  if( evlen >= fBufsize ) {
    delete [] fEvtBuffer;
    fBufsize = TMath::Max( evlen, 2*fBufsize );
    fEvtBuffer = new UInt_t[fBufsize];
  }
  // Copy the buffer.  EPICS events are infrequent, so no harm.
  memcpy( fEvtBuffer, evdata->GetRawDataBuffer(), sizeof(UInt_t)*evlen );

  if (fDebugFile) EvDump(evdata);

  Int_t recent_event = evdata->GetEvNum();
  fEpics->LoadData(fEvtBuffer, recent_event);

  return 1;
}

THaAnalysisObject::EStatus THaEpicsEvtHandler::Init(const TDatime&)
{
  //  cout << "Howdy !  We are initializing THaEpicsEvtHandler !!   name =   "<<fName<<endl;
  
  // Set the event type to the default unless the client has already defined it.
  if (GetNumTypes()==0) SetEvtType(Decoder::EPICS_EVTYPE);

  fStatus = kOK;
  return kOK;
}

ClassImp(THaEpicsEvtHandler)
