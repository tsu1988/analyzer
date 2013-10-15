//*-- Author :    Ole Hansen   08-Oct-08

//////////////////////////////////////////////////////////////////////////
//
// THaPhysicsEventHandler
//
// Standard handler for physics event type
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsEventHandler.h"
#include "TList.h"
#include "THaRunBase.h"
#include "THaEvData.h"
#include "THaOutput.h"
#include "THaBenchmark.h"
#include "THaEvent.h"
#include "THaSpectrometer.h"
#include "THaPhysicsModule.h"

#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaPhysicsEventHandler::THaPhysicsEventHandler() : fFirst(true)
{
  // Normal constructor.

  fCount.SetText( "physics events" );
  fNanalyzed.SetText( "physics events analyzed" );

  fApps    = new TList;
  fPhysics = new TList;

  fDecodeTests      = new Stage( "Decode" );
  fCoarseTrackTests = new Stage( "CoarseTracking" );
  fCoarseReconTests = new Stage( "CoarseReconstruct" );
  fTrackingTests    = new Stage( "Tracking" );
  fReconstructTests = new Stage( "Reconstruct" );
  fPhysicsTests     = new Stage( "Physics" );

}

//_____________________________________________________________________________
THaPhysicsEventHandler::~THaPhysicsEventHandler()
{
  // Destructor

  delete fDecodeTests;
  delete fCoarseTrackTests;
  delete fCoarseReconTests;
  delete fTrackingTests;
  delete fReconstructTests;
  delete fPhysicsTests;

  delete fApps;
  delete fPhysics;
}

//_____________________________________________________________________________
Int_t THaPhysicsEventHandler::Analyze( const THaEvData& evdata, Int_t status )
{
  // Analysis of physics events

  ++fCount;

  // Don't analyze events that did not pass RawDecode cuts
  if( status != THaAnalyzer::kOK )
    return status;

  //--- Skip physics events until we reach the first requested event
  if( GetNev() < GetRun()->GetFirstEvent() )
    return THaAnalyzer::kSkip;

  if( fFirst ) {
    fFirst = false;
    if( GetVerbose() > 2 )
      cout << "Starting physics analysis at event " << fCount << endl;
  }
  // Update counters
  GetRun()->IncrNumAnalyzed();
  ++fNanalyzed;

  //--- Process all apparatuses that are defined in fApps
  //    First Decode(), then Reconstruct()

  if( GetBench() ) GetBench()->Begin("Decode");
  TIter next(fApps);
  while( THaApparatus* theApparatus = static_cast<THaApparatus*>( next() )) {
    theApparatus->Clear();
    theApparatus->Decode( evdata );
  }
  if( GetBench() ) GetBench()->Stop("Decode");
  if( !fDecodeTests->Eval() )  return THaAnalyzer::kSkip;

  //--- Main physics analysis. Calls the following for each defined apparatus
  //    THaSpectrometer::CoarseTrack  (only for spectrometers)
  //    THaApparatus::CoarseReconstruct
  //    THaSpectrometer::Track        (only for spectrometers)
  //    THaApparatus::Reconstruct
  //
  // Test blocks are evaulated after each of these stages

  //-- Coarse processing

  if( GetBench() ) GetBench()->Begin("CoarseTracking");
  next.Reset();
  while( TObject* obj = next() ) {
    THaSpectrometer* theSpectro = dynamic_cast<THaSpectrometer*>(obj);
    if( theSpectro )
      theSpectro->CoarseTrack();
  }
  if( GetBench() ) GetBench()->Stop("CoarseTracking");
  if( !fCoarseTrackTests->Eval() )  return THaAnalyzer::kSkip;
  

  if( GetBench() ) GetBench()->Begin("CoarseReconstruct");
  next.Reset();
  while( THaApparatus* theApparatus =
	 static_cast<THaApparatus*>( next() )) {
    theApparatus->CoarseReconstruct();
  }
  if( GetBench() ) GetBench()->Stop("CoarseReconstruct");
  if( !fCoarseReconTests->Eval() )  return THaAnalyzer::kSkip;

  //-- Fine (Full) Reconstruct().

  if( GetBench() ) GetBench()->Begin("Tracking");
  next.Reset();
  while( TObject* obj = next() ) {
    THaSpectrometer* theSpectro = dynamic_cast<THaSpectrometer*>(obj);
    if( theSpectro )
      theSpectro->Track();
  }
  if( GetBench() ) GetBench()->Stop("Tracking");
  if( !fTrackingTests->Eval() )  return THaAnalyzer::kSkip;
  
  if( GetBench() ) GetBench()->Begin("Reconstruct");
  next.Reset();
  while( THaApparatus* theApparatus = static_cast<THaApparatus*>( next() )) {
    theApparatus->Reconstruct();
  }
  if( GetBench() ) GetBench()->Stop("Reconstruct");
  if( !fReconstructTests->Eval() )  return THaAnalyzer::kSkip;

  //--- Process the list of physics modules

  if( GetBench() ) GetBench()->Begin("Physics");
  TIter next_physics(fPhysics);
  while( THaPhysicsModule* theModule =
	 static_cast<THaPhysicsModule*>( next_physics() )) {
    theModule->Clear();
    Int_t err = theModule->Process( evdata );
    if( err == THaPhysicsModule::kTerminate )
      status = THaAnalyzer::kTerminate;
    else if( err == THaPhysicsModule::kFatal ) {
      status = THaAnalyzer::kFatal;
      break;
    }
  }
  if( GetBench() ) GetBench()->Stop("Physics");
  if( status == THaAnalyzer::kFatal ) return status;

  //--- Evaluate "Physics" test block

  if( !fPhysicsTests->Eval() )
    // any status code form the physics analysis overrides skip code
    return (status == THaAnalyzer::kOK) ? THaAnalyzer::kSkip : status;

  //--- If Event defined, fill it.
  if( GetBench() ) GetBench()->Begin("Output");
  if( GetEvent() ) {
    // TODO: set target helicity, too?
    GetEvent()->GetHeader()->Set( static_cast<UInt_t>(evdata.GetEvNum()),
				  evdata.GetEvType(),
				  evdata.GetEvLength(),
				  evdata.GetEvTime(),
				  evdata.GetHelicity(),
				  GetRun()->GetNumber()
				  );
    GetEvent()->Fill();
  }

  //---  Process output
  if( GetOutput() )
    GetOutput()->Process();

  if( GetBench() ) GetBench()->Stop("Output");

  return status;
}

//_____________________________________________________________________________
Int_t THaPhysicsEventHandler::Begin()
{
  // Begin physics analysis. Called once, before starting the event loop
  // for each run.
  // Executes Begin() for all Apparatus and Physics modules.

  fFirst = true;

  TIter nexta(fApps);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nexta()) ) {
    obj->Begin( GetRun() );
  }
  TIter nextp(fPhysics);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nextp()) ) {
    obj->Begin( GetRun() );
  }

  return 0;
}

//_____________________________________________________________________________
void THaPhysicsEventHandler::Clear( Option_t* opt )
{
  // Clear counters of this event type

  THaEventTypeHandler::Clear(opt);
  fNanalyzed = 0;

  //TODO: clear counters in Analyzer? What do we do here?
}

//_____________________________________________________________________________
Int_t THaPhysicsEventHandler::Init( const TDatime& date,
				    const TList*   module_list )
{
  // Initialize physics event handler.
  // Extracts Apparatus and Physics modules from "modules" list.

  static const char* const here = "Init";

  fApps->Clear();
  fPhysics->Clear();

  if( !module_list )
    return -1;

  // Extract the modules that we want to handle and initialize them
  Int_t retval = 0;
  TIter next(module_list);
  while( TObject* obj = next() ) {

    if( obj->IsZombie() ) {
      Warning( here, "Zombie module %s (%s) ignored",
	       obj->GetName(), obj->GetTitle() );
      continue;
    }
    // Pick out THaApparatus and THaPhysicsModule objects
    THaAnalysisObject* theModule;
    if( (theModule = dynamic_cast<THaApparatus*>(obj)) ) {
      fApps->Add( theModule );
    }
    else if( (theModule = dynamic_cast<THaPhysicsModule*>(obj)) ) {
      fPhysics->Add( theModule );
    }
    // Initialize each module
    if( theModule ) {
      retval = theModule->Init( date );
      if( retval != THaAnalysisObject::kOK || !theModule->IsOK() ) {
	Error( here, "Error %d initializing module %s (%s). Analyzer initial"
	       "ization failed.", retval, obj->GetName(), obj->GetTitle() );
	if( retval == THaAnalysisObject::kOK )
	  retval = -1;
	// Die on error
	break;
      }
    }
  }

  return retval;
}

//_____________________________________________________________________________
Int_t THaPhysicsEventHandler::InitLevel2()
{
  // Level 2 initialization - after global variables and tests/cuts are
  // available.
  //
  // - Init() our Stages
  // - Init output of our analysis modules

  fDecodeTests->Init( GetCuts() );
  fCoarseTrackTests->Init( GetCuts() );
  fCoarseReconTests->Init( GetCuts() );
  fTrackingTests->Init( GetCuts() );
  fReconstructTests->Init( GetCuts() );
  fPhysicsTests->Init( GetCuts() );

  // call the apparatuses again, to permit them to write more
  // complex output directly to the TTree
  //  (retval = InitOutput(fApps)) || (retval = InitOutput(fPhysics));

  return 0;
}

#if 0
//_____________________________________________________________________________
Int_t THaAnalyzer::InitOutput( const TList* module_list )
{
  // Initialize a list of THaAnalysisObject's for output
  static const char* const here = "InitOutput()";

  if( !module_list )
    return -10;
  TIter next( module_list );
  Int_t retval = 0;
  TObject* obj;
  while( (obj = next())) {
    THaAnalysisObject* theModule = dynamic_cast<THaAnalysisObject*>( obj );
    if( !theModule ) {
      Error( here, "Object %s (%s) is not a THaAnalysisObject?!? Output "
	     "initialization failed.", obj->GetName(), obj->GetTitle() );
      retval = -11;
      break;
    }

    theModule->InitOutput( fOutput );

    if( !theModule->IsOKOut() ) {
      Error( here, "Error initializing output for  %s (%s). "
	     "Output initialization failed.", 
	     obj->GetName(), obj->GetTitle() );
      retval = -12;
      break;
    }
  }
  return retval;
}
#endif

//_____________________________________________________________________________
Int_t THaPhysicsEventHandler::End()
{
  // End physics analysis. Called once, after exiting the event loop for 
  // each run.
  // Executes End() for all Apparatus and Physics modules. 

  TIter nexta(fApps);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nexta()) ) {
    obj->End( GetRun() );
  }
  TIter nextp(fPhysics);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nextp()) ) {
    obj->End( GetRun() );
  }

  return 0;
}

//_____________________________________________________________________________
void THaPhysicsEventHandler::Print( Option_t* ) const
{
  // Print info/statistics about this event type

  //TODO
}

//_____________________________________________________________________________
ClassImp(THaPhysicsEventHandler)



