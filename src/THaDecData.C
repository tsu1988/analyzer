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
#include "THaGlobals.h"
#include "TDatime.h"
#include "TClass.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "BdataLoc.h"
#include "THaEvData.h"

#include <iostream>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <memory>

#define DECDATA_LEGACY_DB

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
  RemoveVariables();
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
Int_t THaDecData::DefineLocType( const BdataLoc::BdataLocType& loctype,
				 const TString& configstr, bool re_init )
{
  // Define variables for given loctype using parameters in configstr

  const char* const here = __FUNCTION__;

  Int_t err = 0;

  // Split the string from the database into values separated by commas,
  // spaces, and/or tabs
  auto_ptr<TObjArray> config( configstr.Tokenize(", \t") );
  if( !config->IsEmpty() ) {
    Int_t nparams = config->GetLast()+1;
    assert( nparams > 0 );   // else bug in IsEmpty() or GetLast()
      
    if( nparams % loctype.fNparams != 0 ) {
      Error( Here(here), "Incorrect number of parameters in database key "
	     "%s%s. Have %d, but must be a multiple of %d. Fix database.",
	     GetPrefix(), loctype.fDBkey, nparams, loctype.fNparams );
      return -1;
    }

    TObjArray* params = config.get();
    for( Int_t ip = 0; ip < nparams; ip += loctype.fNparams ) {
      // Prepend prefix to name in parameter array
      TString& bname = GetObjArrayString( params, ip );
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
	    return -1;
	  }
	  if( fDebug>2 ) 
	    Info( Here(here), "Updating variable %s", bname.Data() );
	} else {
	  // Duplicate variable name (i.e. duplicate names in database)
	  Error( Here(here), "Duplicate variable name %s. Fix database.",
		 item->GetName() );
	  return -1;
	}
      } else {
	// Make a new BdataLoc
	if( fDebug>2 )
	  Info( Here(here), "Defining new variable %s",  bname.Data() );
	item = static_cast<BdataLoc*>( loctype.fTClass->New() );
	if( !item ) {
	  Error( Here(here), "Failed to create variable of type %s. Should "
		 "never happen. Call expert.", loctype.fTClass->GetName() );
	  return -1;
	}
      }
      // Configure the new or existing BdataLoc with current database parameters. 
      // The first parameter is always the name. Note that this object's prefix
      // was already prepended above.
      err = item->Configure( params, ip );
      if( !err && loctype.fOptptr != 0 ) {
	// Optional pointer to some type-specific data
	err = item->OptionPtr( loctype.fOptptr );
      }
      if( err ) {
	Int_t in = ip - (ip % loctype.fNparams); // index of name
	Error( Here(here), "Illegal parameter for variable %s, "
	       "index = %d, value = %s. Fix database.",
	       GetObjArrayString(params,in).Data(), ip,
	       GetObjArrayString(params,ip).Data() );
	if( !already_defined )
	  delete item;
	break;
      } else if( !already_defined ) {
	// Add this BdataLoc to the list to be processed
	fBdataLoc.Add(item);
      }
    }
  } else {
    Warning( Here(here), "Empty database key %s%s.",
	     GetPrefix(), loctype.fDBkey );
  }

  return err;
}

//_____________________________________________________________________________
FILE* THaDecData::OpenFile( const TDatime& date )
{ 
  // Open DecData database file. First look for standard file name,
  // e.g. "db_D.dat", then for legacy file name "decdata.map"

  FILE* fi = THaApparatus::OpenFile( date );
  if( fi )
    return fi;
  return THaAnalysisObject::OpenFile("decdata.dat", date,
				     Here("OpenFile()"), "r", fDebug);
}

//_____________________________________________________________________________
#ifdef DECDATA_LEGACY_DB
#include <map>

static Int_t CheckDBVersion( FILE* file )
{
  // Check database version. Similar to emacs mode specs, versions are
  // determined by an identifier comment "# Version: 2" on the first line
  // (within first 80 chars). If no such tag is found, version 1 is assumed.

  const TString identifier("Version:");

  const size_t bufsiz = 82;
  char* buf = new char[bufsiz];
  rewind(file);
  const char* s = fgets(buf,bufsiz,file);
  if( !s ) // No first line? Not our problem...
    return 1;
  TString line(buf);
  delete [] buf;
  Ssiz_t pos = line.Index(identifier,0,TString::kIgnoreCase);
  if( pos == kNPOS )
    return 1;
  pos += identifier.Length();
  while( pos < line.Length() && isspace(line(pos)) ) pos++;
  if( pos >= line.Length() )
    return 1;
  TString line2 = line(pos,line.Length() );
  Int_t vers = line2.Atoi();
  if( vers == 0 )
    vers = 1;
  return vers;
}

//_____________________________________________________________________________
inline
static TString& GetString( const TObjArray* params, Int_t pos )
{
  return THaAnalysisObject::GetObjArrayString(params,pos);
}

//_____________________________________________________________________________
static Int_t ReadOldFormatDB( FILE* file, map<TString,TString>& configstr_map )
{
  // Read old-style THaDecData database file and put results into a map from
  // database key to value (simulating the new-style key/value database info).
  // Old-style "crate" objects are all assumed to be multihit channels, even
  // though they usually are not.

  const size_t bufsiz = 256;
  char* buf = new char[bufsiz];
  string dbline;
  const int nkeys = 3;
  TString confkey[nkeys] = { "multi", "word", "bit" };
  TString confval[nkeys];
  // Read all non-comment lines
  rewind(file);
  while( THaAnalysisObject::ReadDBline(file, buf, bufsiz, dbline) != EOF ) {
    if( dbline.empty() ) continue;
    // Tokenize each line read
    TString line( dbline.c_str() );
    TObjArray* params = line.Tokenize(" \t");
    if( params->IsEmpty() || params->GetLast() < 4 ) continue;
    // Determine data type
    bool is_slot = ( GetString(params,1) == "crate" );
    int idx = is_slot ? 0 : 1;
    TString name = GetString(params,0);
    // TrigBits are crate types with special names
    if( is_slot && name.BeginsWith("bit") ) {
      if( name.Length() > 3 ) {
	TString name2 = name(3,name.Length());
	if( name2.IsDigit() && name2.Atoi() < 32 )
	  idx = 2;
      }
    }
    confval[idx] += name;
    confval[idx] += " ";
    for( int i = 2; i < 5; ++ i ) {
      confval[idx] += GetString(params,i).Data();
      confval[idx] += " ";
    }
  }
  delete [] buf;
  // Put the retrieved strings into the key/value map
  for( int i = 0; i < nkeys; ++ i ) {
    if( !confval[i].IsNull() )
      configstr_map[confkey[i]] = confval[i];
  }
  return 0;
}
#endif

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

#ifdef DECDATA_LEGACY_DB
  bool old_format = (CheckDBVersion(file) == 1);
  map<TString,TString> configstr_map;
  if( old_format )
    ReadOldFormatDB( file, configstr_map );
#endif

  Int_t err = 0;
  for( BdataLoc::TypeIter_t it = BdataLoc::fgBdataLocTypes().begin();
       !err && it != BdataLoc::fgBdataLocTypes().end(); ++it ) {
    const BdataLoc::BdataLocType& loctype = *it;

    // Get the ROOT class for this type
    assert( loctype.fClassName && *loctype.fClassName );
    if( !loctype.fTClass ) {
      loctype.fTClass = TClass::GetClass( loctype.fClassName );
      if( !loctype.fTClass ) {
	// Probably typo in the call to BdataLoc::DoRegister
	Error( Here(here), "No class defined for data type \"%s\". Programming "
	       "error. Call expert.", loctype.fClassName );
	err = -1;
	break;
      }
    }
    if( !loctype.fTClass->InheritsFrom( BdataLoc::Class() )) {
      Error( Here(here), "Class %s is not a BdataLoc. Programming error. "
	     "Call expert.", loctype.fTClass->GetName() );
      err = -1;
      break;
    }

    TString configstr;
#ifdef DECDATA_LEGACY_DB
    // Retrieve old-format database parameters read above for this type
    if( old_format ) {
      map<TString,TString>::const_iterator found =
	configstr_map.find(loctype.fDBkey);
      if( found == configstr_map.end() )
	continue;
      configstr = found->second;
    } else
#endif
    {
      // Read key/value database format
      TString dbkey = loctype.fDBkey;
      dbkey.Prepend( GetPrefix() );
      if( LoadDBvalue( file, date, dbkey, configstr ) != 0 )
	continue;  // No definitions in database for this BdataLoc type
    }
    err = DefineLocType( loctype, configstr, re_init );
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

  if( evtypebits != 0 ) {
    cout << " trigger bits set = ";
    bool cont = false;
    for( UInt_t i = 0; i < sizeof(evtypebits)*kBitsPerByte-1; ++i ) {
      if( TESTBIT(evtypebits,i) ) {
	if( cont ) cout << ", ";
	else       cont = true;
	cout << i;
      }
    }
    cout << endl;
  }

  // Print variables in the order they were defined
  cout << " number of user variables: " << fBdataLoc.GetSize() << endl;
  TIter next( &fBdataLoc );
  while( TObject* obj = next() ) {
    obj->Print(opt);
  }
}

//_____________________________________________________________________________
ClassImp(THaDecData)

