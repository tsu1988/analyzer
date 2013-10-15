#ifndef ROOT_THaAnalyzer
#define ROOT_THaAnalyzer

//////////////////////////////////////////////////////////////////////////
//
// THaAnalyzer
// 
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TString.h"
#include <vector>

class THaEvent;
class THaRunBase;
class THaOutput;
class TList;
class TIter;
class TFile;
class TDatime;
class THaCut;
class THaBenchmark;
class THaEvData;
class THaPostProcess;
class THaCrateMap;
class THaCutList;

using std::vector;

class THaAnalyzer : public TObject {

  friend class THaEventTypeHandler;
  
public:
  THaAnalyzer( Bool_t default_handlers = kTRUE );
  virtual ~THaAnalyzer();

  //TODO: make synonym of AddModule()
  virtual Int_t  AddPostProcess( THaPostProcess* module );
  //TODO: AddModule() function to add analysis modules,
  // also RemoveModules(), DeleteModules(), ... ?
  virtual void   Close();
  virtual Int_t  Init( THaRunBase* run );
          Int_t  Init( THaRunBase& run )    { return Init( &run ); }
  virtual Int_t  Process( THaRunBase* run=NULL );
          Int_t  Process( THaRunBase& run ) { return Process(&run); }
  virtual void   Print( Option_t* opt="" ) const;

  void           EnableBenchmarks( Bool_t b = kTRUE );
  void           EnableHelicity( Bool_t b = kTRUE );
  void           EnableOtherEvents( Bool_t b = kTRUE );
  void           EnableOverwrite( Bool_t b = kTRUE );
  //TODO: implement
  void           EnablePhysicsEvents( Bool_t b = kTRUE );
  void           EnableRunUpdate( Bool_t b = kTRUE );
  void           EnableScalers( Bool_t b = kTRUE );
  void           EnableSlowControl( Bool_t b = kTRUE );

  const char*    GetOutFileName()      const  { return fOutFileName.Data(); }
  const char*    GetCutFileName()      const  { return fCutFileName.Data(); }
  const char*    GetOdefFileName()     const  { return fOdefFileName.Data(); }
  const char*    GetSummaryFileName()  const  { return fSummaryFileName.Data(); }
  TFile*         GetOutFile()          const  { return fFile; }
  Int_t          GetCompressionLevel() const  { return fCompress; }
  THaEvent*      GetEvent()            const  { return fEvent; }
  THaEvData*     GetDecoder()          const  { return fEvData; }
  // TList*         GetApps()             const  { return fApps; }
  // TList*         GetPhysics()          const  { return fPhysics; }
  // TList*         GetScalers()          const  { return fScalers; }
  // TList*         GetPostProcess()      const  { return fPostProcess; }
  //TODO: implement using event type handlers
  TList*         GetApps()             const;
  TList*         GetPhysics()          const;
  TList*         GetScalers()          const;
  TList*         GetPostProcess()      const;
  Bool_t         HasStarted()          const  { return fAnalysisStarted; }
  Bool_t         HelicityEnabled()     const  { return fDoHelicity; }
  // Bool_t         PhysicsEnabled()      const  { return fDoPhysics; }
  // Bool_t         OtherEventsEnabled()  const  { return fDoOtherEvents; }
  // Bool_t         ScalersEnabled()      const  { return fDoScalers; }
  // Bool_t         SlowControlEnabled()  const  { return fDoSlowControl; }
  //TODO: implement using event type handlers
  Bool_t         PhysicsEnabled()      const;
  Bool_t         OtherEventsEnabled()  const;
  Bool_t         ScalersEnabled()      const;
  Bool_t         SlowControlEnabled()  const;
  virtual Int_t  SetCountMode( Int_t mode );
  void           SetCrateMapFileName( const char* name );
  void           SetEvent( THaEvent* event )     { fEvent = event; }
  void           SetOutFile( const char* name )  { fOutFileName = name; }
  void           SetCutFile( const char* name )  { fCutFileName = name; }
  void           SetOdefFile( const char* name ) { fOdefFileName = name; }
  void           SetSummaryFile( const char* name ) { fSummaryFileName = name; }
  void           SetCompressionLevel( Int_t level ) { fCompress = level; }
  void           SetMarkInterval( UInt_t interval ) { fMarkInterval = interval; }
  void           SetVerbosity( Int_t level )        { fVerbose = level; }

  static THaAnalyzer* GetInstance() { return fgAnalyzer; }

  // Pre-defined modes for the event counter fNev. Determines range of event
  // numbers that is analyzed - see SetCountMode().
  enum ECountMode { kCountPhysics, kCountAll, kCountRaw };

  // Return codes for analysis routines inside event loop
  enum ERetVal { kOK, kSkip, kTerminate, kFatal };

protected:
    
  //___________________________________________________________________________
  // Helper class - basic statistics counter
  // Essentially an unsigned integer with a description text
  class Counter {
  public:
    Counter( const char* text="" ) : fCount(0), fText(text) {}
    
    UInt_t operator++() { return ++fCount; }
    const UInt_t operator++(int) { return fCount++; }
    UInt_t operator--() { return --fCount; }
    const UInt_t operator--(int) { return fCount--; }
    operator UInt_t() const { return fCount; }
    UInt_t& operator=( const UInt_t rhs ) { fCount = rhs; return fCount; }

    UInt_t      GetCount() const             { return fCount; }
    const char* GetText()  const             { return fText.Data(); }
    void        Print( UInt_t w = 0 ) const;
    void        SetText( const char* text )  { fText = text; }

  private:
    UInt_t      fCount;       // Counter value
    TString     fText;        // Description
  };

  //___________________________________________________________________________
  // Helper class for processing of cuts and histograms at key points
  // in the analysis chain
  class Stage {
  public:
    Stage( const char* name );
    virtual ~Stage() {}

    virtual void   Clear() { fNeval = 0; fNfail = 0; }
    virtual void   Init( THaCutList* cut_list );
    virtual Int_t  Eval();
    virtual void   Print( Option_t* opt="" ) const;

  private:
    TString      fName;        // Name. Determines which hists/cuts used
    TList*       fHistList;    // TODO: List of histograms to fill
    TList*       fCutList;     // List of cuts to evaluate
    THaCut*      fMasterCut;   // If defined, abort event if this cut is failed
    Counter      fNeval;       // Number of times Eval() was called 
    Counter      fNfail;       // Number of events that failed fMasterCut
  };
    
  //___________________________________________________________________________

  TFile*         fFile;            // ROOT output file.
  THaOutput*     fOutput;          //Flexible ROOT output (tree, histograms)
  THaCutList*    fCuts;            // Cut/test definitions loaded
  TString        fOutFileName;     //Name of output ROOT file.
  TString        fCutFileName;     //Name of cut definition file to load
  TString        fLoadedCutFileName;//Name of last loaded cut definition file
  TString        fOdefFileName;    //Name of output definition file
  TString        fSummaryFileName; //Name of test/cut statistics output file
  THaEvent*      fEvent;           // Event structure to be written to file.
  UInt_t         fNev;             // # events read during most recent replay
  UInt_t         fMarkInterval;    //Interval for printing event numbers
  Int_t          fCompress;        //Compression level for ROOT output file
  Int_t          fVerbose;         //Verbosity level
  Int_t          fCountMode;       //Event counting mode (see ECountMode)
  THaBenchmark*  fBench;           //Counters for timing statistics
  THaEvent*      fPrevEvent;       //Event structure from last Init()
  THaRunBase*    fRun;             //Pointer to current run
  THaEvData*     fEvData;          //Instance of decoder used by us
  //TODO: these are now part of event type handlers
//   TList*         fApps;            // List of apparatuses
//   TList*         fPhysics;         // List of physics modules
//   TList*         fScalers;         // List of scaler groups
//   TList*         fPostProcess;     // List of post-processing modules
  TList*         fModules;         // List of THaAnalysisModules to use
  TList*         fEvTypeHandlers;  // List of THaEventTypeHandlers to use

  // Status and control flags
  Bool_t         fIsInit;          // Init() called successfully
  Bool_t         fAnalysisStarted; // Process() run and output file open
  Bool_t         fLocalEvent;      // fEvent allocated by this object
  Bool_t         fUpdateRun;       // Update run parameters during replay
  Bool_t         fOverwrite;       // Overwrite existing output files
  Bool_t         fDoHelicity;      // Enable helicity decoding

  // Variables used by analysis functions
  Bool_t         fFirstPhysics;    // Status flag for physics analysis


  // Main analysis functions
  virtual Int_t  BeginAnalysis();
  virtual Int_t  DoInit( THaRunBase* run );
  virtual Int_t  EndAnalysis();
  virtual Int_t  MainAnalysis();
  virtual Int_t  ReadOneEvent();

  // Support methods
  void           ClearCounters();
  void           DefineCounters();
  void           DefineModules();
  void           DefineStages();
  //  UInt_t         GetCount( UInt_t which ) const;
  //  UInt_t         Incr( UInt_t which );
  virtual void   InitStages();
  virtual void   PrintCounters() const;
  //  virtual void   PrintScalers() const;
  virtual void   PrintCutSummary() const;

  static THaAnalyzer* fgAnalyzer;  //Pointer to instance of this class

  // In-class constants
  static const char* const kMasterCutName;
  static const char* const kDefaultOdefFile;

private:
  THaAnalyzer( const THaAnalyzer& );
  THaAnalyzer& operator=( const THaAnalyzer& );
  
  ClassDef(THaAnalyzer,0)  //Hall A Analyzer Standard Event Loop

};

#endif
