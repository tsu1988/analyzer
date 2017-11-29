/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//   Hall A Event Data from One "Event"
//
//   This is a pure virtual base class.  You should not
//   instantiate this directly (and actually CAN not), but
//   rather use Decoder::CodaDecoder or (less likely) a sim class like
//   Podd::SimDecoder.
//
//   This class is intended to provide a crate/slot structure
//   for derived classes to use.  All derived class must define and
//   implement LoadEvent(const UInt_t*).  See the header.
//
//   original author  Robert Michaels (rom@jlab.org)
//
//   modified for abstraction by Ken Rossato (rossato@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "Module.h"
#include "THaSlotData.h"
#include "THaFastBusWord.h"
#include "THaCrateMap.h"
#include "THaUsrstrutils.h"
#include "THaBenchmark.h"
#include "TError.h"
#include <cstring>
#include <cstdio>
#include <cctype>
#include <iomanip>
#include <ctime>

#ifndef STANDALONE
#include "THaVarList.h"
#include "THaGlobals.h"
#endif

using namespace std;
using namespace Decoder;

// Instances of this object
TBits THaEvData::fgInstances;

const Double_t THaEvData::kBig = 1e38;

// If false, signal attempted use of unimplemented features
#ifndef NDEBUG
Bool_t THaEvData::fgAllowUnimpl = false;
#else
Bool_t THaEvData::fgAllowUnimpl = true;
#endif

TString THaEvData::fgDefaultCrateMapName = "cratemap";

//_____________________________________________________________________________
THaEvData::THaEvData() :
  fMap(0), first_decode(true), fTrigSupPS(true),
  fMultiBlockMode(kFALSE), fBlockIsDone(kFALSE),
  buffer(0), run_num(0), run_type(0), fRunTime(0),
  evt_time(0), recent_event(0), fNSlotUsed(0), fNSlotClear(0),
  fDoBench(kFALSE), fBench(0), fNeedInit(true), fDebug(0),
  fDebugFile(0)
{
  fInstance = fgInstances.FirstNullBit();
  fgInstances.SetBitNumber(fInstance);
  fInstance++;
  // FIXME: dynamic allocation
  crateslot = new THaSlotData*[MAXROC*MAXSLOT];
  fSlotUsed  = new UShort_t[MAXROC*MAXSLOT];
  fSlotClear = new UShort_t[MAXROC*MAXSLOT];
  //memset(psfact,0,MAX_PSFACT*sizeof(int));
  memset(crateslot,0,MAXROC*MAXSLOT*sizeof(THaSlotData*));
  fRunTime = time(0); // default fRunTime is NOW
  fEpicsEvtType = Decoder::EPICS_EVTYPE;  // default for Hall A
#ifndef STANDALONE
// Register global variables.
  if( gHaVars ) {
    VarDef vars[] = {
      { "runnum",    "Run number",     kInt,    0, &run_num },
      { "runtype",   "CODA run type",  kInt,    0, &run_type },
      { "runtime",   "CODA run time",  kULong,  0, &fRunTime },
      { "evnum",     "Event number",   kInt,    0, &event_num },
      { "evtyp",     "Event type",     kInt,    0, &event_type },
      { "evlen",     "Event Length",   kInt,    0, &event_length },
      { "evtime",    "Event time",     kULong,  0, &evt_time },
      { 0 }
    };
    TString prefix("g");
    // Prevent global variable clash if there are several instances of us
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".");
    gHaVars->DefineVariables( vars, prefix, "THaEvData::THaEvData" );
  } else
    Warning("THaEvData::THaEvData","No global variable list found. "
            "Variables not registered.");
#endif
}


//_____________________________________________________________________________
THaEvData::~THaEvData()
{
  if( fDoBench ) {
    Float_t a,b;
    fBench->Summary(a,b);
  }
  delete fBench;
#ifndef STANDALONE
  if( gHaVars ) {
    TString prefix("g");
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".*");
    gHaVars->RemoveRegexp( prefix );
  }
#endif
  // We must delete every array element since not all may be in fSlotUsed.
  for( int i=0; i<MAXROC*MAXSLOT; i++ )
    delete crateslot[i];
  delete [] crateslot;
  delete [] fSlotUsed;
  delete [] fSlotClear;
  delete fMap;
  delete fDebugFile;
  fInstance--;
  fgInstances.ResetBitNumber(fInstance);
}

//_____________________________________________________________________________
const char* THaEvData::DevType(int crate, int slot) const
{
  // Device type in crate, slot
  return ( GoodIndex(crate,slot) ) ?
    crateslot[idx(crate,slot)]->devType() : " ";
}

//_____________________________________________________________________________
Int_t THaEvData::Init()
{
  Int_t ret = HED_OK;
  ret = init_cmap();
  if (fMap) fMap->print();
  if (ret != HED_OK) return ret;
  ret = init_slotdata(fMap);
  first_decode = kFALSE;
  fNeedInit = kFALSE;
  return ret;
}

//_____________________________________________________________________________
void THaEvData::SetDebugFile( const char* filename, bool append )
{
  // Send debug output to the given file. The file will be overwritten
  // upon calling this routine unless 'append' is set.
  // Calling this function will turn on all debug messages by default, equivalent
  // to calling SetDebug(255).  Call SetDebug(level) with a lower level if you
  // want less output.
  // To close the file, call this routine with an empty string, which
  // will also turn off all debug messages.
  
  const char* const here = "THaEvData";
  
  if( !filename ) {
    Error( here, "Invalid file name for debug output. Ignoring." );
    return;
  }
  delete fDebugFile; fDebugFile = 0;
  if( *filename ) {
    ios_base::openmode mode = ios_base::out;
    if( append )
      mode |= ios_base::app;
    fDebugFile = new ofstream(filename,mode);
    if( !fDebugFile || !*fDebugFile ) {
      Error( here, "Cannot open debug output file %s. "
             "Check file name and permissions.", filename );
      delete fDebugFile; fDebugFile = 0;
      return ;
    }
    SetDebug(255);
    Info( here, "Sending debug output to \"%s\".", filename );
  } else {
    SetDebug(0);
    Info( here, "Debug off." );
  }
}

//_____________________________________________________________________________
void THaEvData::SetDebugFile( ofstream* fstream )
{
  // Deprecated function to set debug output to the given output file stream.
  // Use SetDebugFile("filename.txt") instead.
  // This class will take ownership of the fiven object and will delete it
  // upon destruction. The caller must not delete the fstream object themselves.
  // Calling this function will turn on all debug messages by default, equivalent
  // to calling SetDebug(255).  Call SetDebug(level) with a lower level if you
  // want less output.
  
  SetDebug(255);
  fDebugFile = fstream;
}

//_____________________________________________________________________________
void THaEvData::SetRunTime( ULong64_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.
  if( fRunTime == tloc )
    return;
  fRunTime = tloc;

  init_cmap();
}

//_____________________________________________________________________________
void THaEvData::EnableBenchmarks( Bool_t enable )
{
  // Enable/disable run time reporting
  fDoBench = enable;
  if( fDoBench ) {
    if( !fBench )
      fBench = new THaBenchmark;
  } else {
    delete fBench;
    fBench = 0;
  }
}

//_____________________________________________________________________________
void THaEvData::EnableHelicity( Bool_t enable )
{
  // Enable/disable helicity decoding
  SetBit(kHelicityEnabled, enable);
}

//_____________________________________________________________________________
void THaEvData::EnableScalers( Bool_t enable )
{
  // Enable/disable scaler decoding
  SetBit(kScalersEnabled, enable);
}

//_____________________________________________________________________________
void THaEvData::SetVerbose( UInt_t level )
{
  // Set verbosity level. Identical to SetDebug(). Kept for compatibility.

  SetDebug(level);
}

//_____________________________________________________________________________
void THaEvData::SetDebug( UInt_t level )
{
  // Set debug level

  fDebug = level;
}

//_____________________________________________________________________________
void THaEvData::SetOrigPS(Int_t evtyp)
{
  fTrigSupPS = true;  // default after Nov 2003
  if (evtyp == PRESCALE_EVTYPE) {
    fTrigSupPS = false;
    return;
  } else if (evtyp != TS_PRESCALE_EVTYPE) {
    cout << "SetOrigPS::Warn: PS factors";
    cout << " originate only from evtype ";
    cout << PRESCALE_EVTYPE << "  or ";
    cout << TS_PRESCALE_EVTYPE << endl;
  }
}

//_____________________________________________________________________________
TString THaEvData::GetOrigPS() const
{
  TString answer = "PS from ";
  if (fTrigSupPS) {
    answer += " Trig Sup evtype ";
    answer.Append(Form("%d",TS_PRESCALE_EVTYPE));
  } else {
    answer += " PS evtype ";
    answer.Append(Form("%d",PRESCALE_EVTYPE));
  }
  return answer;
}

//_____________________________________________________________________________
void THaEvData::hexdump(const char* cbuff, size_t nlen, ostream& os)
{
  // Hexdump buffer 'cbuff' of length 'nlen' bytes to output stream 'os'

  if( !cbuff || nlen == 0 ) return;
  const size_t NW = 16, GRN = 2; const char* p = cbuff;
  // Determine how many digits we need for the address offset
  int sh = sizeof(nlen)*8;
  const size_t mask = (size_t)1ULL<<(sh-1);
  size_t n = nlen, lw, ld = 0;
  while( (n&mask)==0 ) { n<<=1; --sh; }
  lw = (sh/(4*GRN)+1)*GRN;
  n = nlen;
  while( n>0 ) { n/=10; ++ld; }
  // Print <offset> <16 data words (16 bytes)> <ASCII representation>
  while( p<cbuff+nlen ) {
    size_t off = p-cbuff;
    os << "0x"
       << hex << setw(lw) << setfill('0') << off << "/"
       << dec << setw(ld) << setfill(' ') << left << off
       << hex << right << " ";
    size_t nelem = TMath::Min((size_t)NW,nlen-off);
    for(size_t i=0; i<NW; i++) {
      UInt_t c = (i<nelem) ? *(const unsigned char*)(p+i) : 0;
      os << " " << setfill('0') << setw(2) << c;
      if( i+1==NW/2 ) os << " ";
    } os << setfill(' ') << "  |" << dec;
    for(size_t i=0; i<NW; i++) {
      char c = (i<nelem) ? *(p+i) : 0;
      if(isgraph(c)||c==' ') os << c; else os << ".";
    } os << "|" << endl;
    p += NW;
  }
}

//_____________________________________________________________________________
void THaEvData::hexdump(const char* cbuff, size_t nlen)
{
  // Hexdump buffer 'cbuff' of length 'nlen' to stdout
  hexdump(cbuff,nlen,std::cout);
}

//_____________________________________________________________________________
void THaEvData::dump( ostream& os ) const
{
  // Dumps data to ostream 'os'. Must have called LoadEvent first.

  if( !buffer ) { //FIXME: this is not the correct test!
    Error("THaEvData::dump", "Must call LoadEvent first");
    return;
  }
  os << endl <<" ------ Raw Data Dump ------ " << endl;
  os << endl << endl << " Event number " << dec << GetEvNum() << endl;
  os << " length " << GetEvLength() << " type " << GetEvType() << endl;

  hexdump(reinterpret_cast<const char*>(buffer), 4*event_length, os);
}

//_____________________________________________________________________________
void THaEvData::SetDefaultCrateMapName( const char* name )
{
  // Static function to set fgDefaultCrateMapName. Call this function to set a
  // global default name for all decoder instances before initialization. This
  // is usually what you want to do for a given replay.

  if( name && *name ) {
    fgDefaultCrateMapName = name;
  }
  else {
    ::Error( "THaEvData::SetDefaultCrateMapName", "Default crate map name "
             "must not be empty" );
  }
}

//_____________________________________________________________________________
void THaEvData::SetCrateMapName( const char* name )
{
  // Set fCrateMapName for this decoder instance only

  if( name && *name ) {
    if( fCrateMapName != name ) {
      fCrateMapName = name;
      fNeedInit = true;
    }
  } else if( fCrateMapName != fgDefaultCrateMapName ) {
    fCrateMapName = fgDefaultCrateMapName;
    fNeedInit = true;
  }
}

//_____________________________________________________________________________
// Set up and initialize the crate map
int THaEvData::init_cmap()
{
  if( fCrateMapName.IsNull() )
    fCrateMapName = fgDefaultCrateMapName;
  if( !fMap || fNeedInit || fCrateMapName != fMap->GetName() ) {
    delete fMap;
    fMap = new THaCrateMap( fCrateMapName );
  }
  if( fDebug>0 ) cout << "Init crate map " << endl;
  if( fMap->init(GetRunTime()) == THaCrateMap::CM_ERR )
    return HED_FATAL; // Can't continue w/o cratemap
  fNeedInit = false;
  return HED_OK;
}

//_____________________________________________________________________________
void THaEvData::makeidx(int crate, int slot)
{
  // Activate crate/slot
  int idx = slot+MAXSLOT*crate;
  delete crateslot[idx];  // just in case
  crateslot[idx] = new THaSlotData(crate,slot);
  if (fDebugFile) crateslot[idx]->SetDebugFile(fDebugFile);
  if( !fMap ) return;
  if( fMap->crateUsed(crate) && fMap->slotUsed(crate,slot)) {
    crateslot[idx]
      ->define( crate, slot, fMap->getNchan(crate,slot),
                fMap->getNdata(crate,slot) );
    fSlotUsed[fNSlotUsed++] = idx;
    if( fMap->slotClear(crate,slot))
      fSlotClear[fNSlotClear++] = idx;
  }
}

//_____________________________________________________________________________
void THaEvData::PrintOut() const
{
  // Print info about this event. Currently just dumps the raw data to stdout

  dump(std::cout);
}

//_____________________________________________________________________________
void THaEvData::PrintSlotData(int crate, int slot) const
{
  // Print the contents of (crate, slot).
  if( GoodIndex(crate,slot)) {
    crateslot[idx(crate,slot)]->print();
  } else {
      cout << "THaEvData: Warning: Crate, slot combination";
      cout << "\nexceeds limits.  Cannot print"<<endl;
  }
  return;
}

//_____________________________________________________________________________
// To initialize the THaSlotData member on first call to decoder
int THaEvData::init_slotdata(const THaCrateMap* map)
{
  // Update lists of used/clearable slots in case crate map changed
  if(!map) return HED_ERR;
  for( int i=0; i<fNSlotUsed; i++ ) {
    THaSlotData* module = crateslot[fSlotUsed[i]];
    int crate = module->getCrate();
    int slot  = module->getSlot();
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot) ||
        !map->slotClear(crate,slot)) {
      for( int k=0; k<fNSlotClear; k++ ) {
        if( module == crateslot[fSlotClear[k]] ) {
          for( int j=k+1; j<fNSlotClear; j++ )
            fSlotClear[j-1] = fSlotClear[j];
          fNSlotClear--;
          break;
        }
      }
    }
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot)) {
      for( int j=i+1; j<fNSlotUsed; j++ )
        fSlotUsed[j-1] = fSlotUsed[j];
      fNSlotUsed--;
    }
  }
  return HED_OK;
}

//_____________________________________________________________________________
void THaEvData::FindUsedSlots()
{
  // Disable slots for which no module is defined.
  // This speeds up the decoder.
  for (Int_t roc=0; roc<MAXROC; roc++) {
    for (Int_t slot=0; slot<MAXSLOT; slot++) {
      if ( !fMap->slotUsed(roc,slot) ) continue;
      if ( !crateslot[idx(roc,slot)]->GetModule() ) {
        cout << "WARNING:  No module defined for crate "<<roc<<"   slot "<<slot<<endl;
        cout << "Check db_cratemap.dat for module that is undefined"<<endl;
        cout << "This crate, slot will be ignored"<<endl;
        fMap->setUnused(roc,slot);
      }
    }
  }
}

//_____________________________________________________________________________
Module* THaEvData::GetModule(Int_t roc, Int_t slot) const
{
  THaSlotData *sldat = crateslot[idx(roc,slot)];
  if (sldat) return sldat->GetModule();
  return NULL;
}

//_____________________________________________________________________________
ClassImp(THaEvData)
ClassImp(THaBenchmark)
