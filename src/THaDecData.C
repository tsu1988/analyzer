//*-- Author:   Ole Hansen, October 2013

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
// Hall A miscellaneous decoder data, which typically does not 
// belong to a detector class.
// Provides global data to analyzer, and
// a place to rapidly add new channels.
//
// Normally the user should have a file "decdata.map" in their pwd
// to define the locations of raw data for this class (only).
// But if this file is not found, we define a default mapping, which
// was valid at least at one point in history.
//
// The scheme is as follows:
//
//    1. In Init() we define a list of global variables which are tied
//       to the variables of this class.  E.g. "timeroc2".
//
//    2. Next we build a list of "BdataLoc" objects which store information
//       about where the data are located.  These data are either directly
//       related to the variables of this class (e.g. timeroc2 is a raw
//       data word) or one must analyze them to obtain a variable.
//
//    3. The BdataLoc objects may be defined by decdata.map which has an
//       obvious notation (see ~/examples/decdata.map).  The entries are either 
//       locations in crates or locations relative to a unique header.
//       If decdata.map is not in the pwd where you run analyzer, then this
//       class uses its own internal DefaultMap().
//
//    4. The BdataLoc objects pertain to one data channel (e.g. a fastbus
//       channel) and and may be multihit.
//
//    5. To add a new variable, if it is on a single-hit channel, you may
//       imitate 'synchadc1' if you know the (crate,slot,chan), and 
//       imitate 'timeroc2' if you know the (crate,header,no-to-skip).
//       If your variable is more complicated and relies on several 
//       channels, imitate the way 'bits' leads to 'evtypebits'.
//
// R. Michaels, March 2002
//
// OR  (NEW as of April 2004:  R.J. Feuerbach)
//       If you are simply interested in the readout of a channel, create
//       a name for it and give the location in the map file and a
//       global variable will be automatically created to monitor that channel.
//     Unfortunately, this leads to a limitation of using arrays as opposed
//     to variable-sized vector for the readout. Currently limited to 16 hits
//     per channel per event.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDecData.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaDetMap.h"
#include "TDatime.h"
#include "TClass.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TDirectory.h"
#include "BdataLoc.h"
#include "THaEvData.h"
#include "VarDef.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <cctype>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <algorithm>

using namespace std;

static Int_t kInitHashCapacity = 100;
static Int_t kRehashLevel = 3;

//_____________________________________________________________________________
THaDecData::THaDecData( const char* name, const char* descript )
  : THaApparatus( name, descript ), evtype(0), evtypebits(0),
    fBdataLoc( kInitHashCapacity, kRehashLevel )
{
  fProperties &= ~kNeedsRunDB;
  fBdataLoc.SetOwner(kTRUE);
}

//_____________________________________________________________________________
THaDecData::~THaDecData()
{
  // Destructor. Delete data location objects and global variables.

  fBdataLoc.Clear();
  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaDecData::Clear( Option_t* )
{
  // Reset event-by-event data

  TIter next( &fBdataLoc );
  while( TObject* obj = next() ) {
    obj->Clear();
  }
  evtype     = 0;
  evtypebits = 0;

}

//_____________________________________________________________________________
Int_t THaDecData::DefineVariables( EMode mode )
{
  // Register global variables, open decdata map file, and parse it.
  // If mode == kDelete, remove global variables.

  // Each defined decoder data location defines its own global variable(s)
  // The BdataLoc have their own equivalent of fIsSetup, so we can
  // unconditionally call their DefineVariables() here (similar to detetcor
  // initialization in THaAppratus::Init).
  Int_t retval = kOK;
  TIter next( &fBdataLoc );
  while( BdataLoc* dataloc = static_cast<BdataLoc*>( next() ) ) {
    if( dataloc->DefineVariables( mode ) != kOK )
      retval = kInitError;
  }
  if( retval != kOK )
    return retval;

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "evtype",     "CODA event type",             "evtype" },  
    { "evtypebits", "event type bit pattern",      "evtypebits" },  
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t THaDecData::ReadDatabase( const TDatime& date )
{
  // Read THaDecData database

  static const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  Bool_t re_init = fIsInit;
  fIsInit = kFALSE;
  if( !re_init ) {
    fBdataLoc.Clear();
  }

  Int_t err = 0;
  for( BdataLoc::TypeIter_t it = BdataLoc::fgBdataLocTypes().begin();
       !err && it != BdataLoc::fgBdataLocTypes().end(); ++it ) {
    const BdataLoc::BdataLocType& loctype = *it;
    TString dbkey = fPrefix, configstr;
    dbkey += loctype.fDBkey;

    Int_t ret = LoadDBvalue( file, date, dbkey, configstr );
    if( ret ) continue;  // No definitions for this BdataLoc type

    TObjArray* params = configstr.Tokenize(" ");
    if( !params->IsEmpty() ) {
      Int_t nparams = params->GetLast()+1;
      assert( nparams > 0 );   // else bug in IsEmpty() or GetLast()
      
      if( nparams % loctype.nparams != 0 ) {
	Error( Here(here), "Incorrect number of parameters in database key "
	       "%s. Have %d, but must be a multiple of %d. Fix database.",
	       dbkey.Data(), nparams, loctype.nparams );
	err = -1;
      }

      for( Int_t ip = 0; ip < nparams; ip += loctype.nparams ) {
	// Prepend prefix to name in parameter array
	TString& bname = BdataLoc::GetString( params, ip );
	bname.Prepend(GetPrefix());
	BdataLoc* item = static_cast<BdataLoc*>(fBdataLoc.FindObject(bname) );
	Bool_t already_defined = ( item != 0 );
	if( already_defined ) {
	  // Name already exists
	  if( re_init ) {
	    // Changing the variable type during a run makes no sense
	    if( loctype.fTClass != item->IsA() ) {
	      Error( Here(here), "Attempt to redefine existing variable %s "
		     "with different type.\nOld = %s, new = %s. Fix database.",
		     item->GetName(), item->IsA()->GetName(),
		     loctype.fTClass->GetName() );
	      err = -1;
	      break;
	    }
	    if( fDebug>2 ) 
	      Info( Here(here), "Updating variable %s", bname.Data() );
	  } else {
	    // Duplicate variable name (i.e. duplicate names in database)
	    Error( Here(here), "Duplicate variable name %s. Fix database.",
		   item->GetName() );
	    err = -1;
	    break;
	  }
	} else {
	  // Make a new BdataLoc
	  if( fDebug>2 )
	    Info( Here(here), "Defining new variable %s",  bname.Data() );
	  item = static_cast<BdataLoc*>( loctype.fTClass->New() );
	  if( !item ) {
	    Error( Here(here), "Failed to create variable of type %s. Should "
		   "never happen. Call expert.", loctype.fTClass->GetName() );
	    err = -1;
	    break;
	  }
	}
	// Configure the new or existing BdataLoc with current database parameters. 
	// The first parameter is always the name. Note that this object's prefix
	// was already prepended above.
	err = item->Configure( params, ip );
	if( !err && loctype.optptr != 0 ) {
	  // Optional pointer to some type-specific data
	  err = item->OptionPtr( loctype.optptr );
	}
	if( err ) {
	  Int_t in = ip - (ip % loctype.nparams); // index of name
	  Error( Here(here), "Illegal parameter for variable %s, "
		 "index = %d, value = %s. Fix database.",
		 BdataLoc::GetString(params,in).Data(), ip,
		 BdataLoc::GetString(params,ip).Data() );
	  if( !already_defined )
	    delete item;
	  break;
	} else if( !already_defined ) {
	  // Add this BdataLoc to the list to be processed
	  fBdataLoc.Add(item);
	}
      }
    }
    delete params;
  }

  fclose(file);
  if( err )
    return kInitError;

// ======= FIXME: Hall A lib ================================================
  // Configure the trigger bits with a pointer to our evtypebits
  TIter next( &fBdataLoc );
  while( BdataLoc* dataloc = static_cast<BdataLoc*>( next() ) ) {
    if( dataloc->Class() == TrigBitLoc::Class() )
      dataloc->OptionPtr( &evtypebits );
  }
// ======= END FIXME: Hall A lib ============================================

  fIsInit = kTRUE;
  return kOK;
}


//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no traditional "detectors",
  // we skip the detector initialization.

  // Standard analysis object init, calls MakePrefix(), ReadDatabase()
  // and DefineVariables(), and Clear("I")
  return THaAnalysisObject::Init( run_time );
}

//_____________________________________________________________________________
Int_t THaDecData::Decode(const THaEvData& evdata)
{
  // Extract the requested variables from the event data

  if( !IsOK() )
    return -1;

  Clear();

  evtype = evdata.GetEvType();   // CODA event type 

  // For each raw data source registered in fBdataLoc, get the data 

  //TODO: accelerate search for multiple header words in same crate
  //- group WordLoc objects by crate
  //- for each crate with >1 WordLoc defs, make a MultiWordLoc
  //- MultiWordLoc uses faster search algo to scan crate buffer

  TIter next( &fBdataLoc );
  while( BdataLoc *dataloc = static_cast<BdataLoc*>( next() ) ) {
    dataloc->Load( evdata );
  }
  
  if( fDebug>1 )
    Print();

  return 0;
}

//_____________________________________________________________________________
void THaDecData::Print( Option_t* opt ) const
{
  // Print current status of all THaDecData variables

  THaAnalysisObject::Print(opt);

  cout << " event types,  CODA = " << evtype
       << "   bit pattern = 0x"      << hex << evtypebits << dec
       << endl;

  // Print variables in the order they were defined
  cout << " number of variables: " << fBdataLoc.GetSize() << endl;
  TIter next( &fBdataLoc );
  while( TObject* obj = next() ) {
    obj->Print(opt);
  }
}

//_____________________________________________________________________________
ClassImp(THaDecData)

