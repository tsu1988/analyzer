//*-- Author :    Bob Michaels,  March 2002

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

typedef vector<BdataLoc*>::iterator Iter_t;

// FIXME: override other THaApparatus functions?

//___________________________________________________________________________
struct DeleteObject {
  template< typename T >
  void operator() ( const T* ptr ) const { delete ptr; }
};

//___________________________________________________________________________
template< typename Container >
inline void DeleteContainer( Container& c )
{
  // Delete all elements of given container of pointers
  for_each( c.begin(), c.end(), DeleteObject() );
  c.clear();
}

//_____________________________________________________________________________
// static UInt_t header_str_to_base16(const char* hdr) {
//   // Utility to convert string header to base 16 (FIXME: base 16??) integer
//   const char chex[] = "0123456789abcdef";
//   if( !hdr ) return 0;
//   const char* p = hdr+strlen(hdr);
//   UInt_t result = 0;  UInt_t power = 1;
//   while( p-- != hdr ) {
//     const char* q = strchr(chex,tolower(*p));
//     if( q ) {
//       result += (q-chex)*power; 
//       power *= 16;
//     }
//   }
//   return result;
// };


//_____________________________________________________________________________
THaDecData::THaDecData( const char* name, const char* descript )
  : THaApparatus( name, descript )
{

}

//_____________________________________________________________________________
THaDecData::~THaDecData()
{
  // Destructor. Delete data location objects and global variables.

  DeleteContainer( fBdataLoc );
  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaDecData::Clear( Option_t* )
{
  // Clear the object (reset event-by-event data)

  THaApparatus::Clear();

  for( Iter_t it = fBdataLoc.begin(); it != fBdataLoc.end(); ++it ) {
    BdataLoc* dataloc = *it;
    dataloc->Clear();
  }

  evtype     = 0;
  evtypebits = 0;

}

//_____________________________________________________________________________
Int_t THaDecData::DefineVariables( EMode mode )
{
  // Register global variables, open decdata map file, and parse it.
  // If mode == kDelete, remove global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "evtype",     "CODA event type",             "evtype" },  
    { "evtypebits", "event type bit pattern",      "evtypebits" },  
    { 0 }
  };
  Int_t retval = DefineVarsFromList( vars, mode );
  if( retval != kOK && mode == kDefine ) {
    // Something went wrong; back out by removing all of our variables
    DefineVariables( kDelete );
    return retval;
  }

  // Each defined decoder data location defines its own global variable(s)
  for( Iter_t it = fBdataLoc.begin(); it != fBdataLoc.end(); ++it ) {
    BdataLoc* dataloc = *it;
    retval = dataloc->DefineVariables( mode );
    if( retval != kOK && mode == kDefine ) {
      DefineVariables( kDelete );
      return retval;
    }
  }
  return retval;
}

//_____________________________________________________________________________
Int_t THaDecData::ReadDatabase( const TDatime& date )
{
  // Read THaDecData database

  static const char* const here = "ReadDatabase";

  fIsInit = kFALSE;

  // Delete existing configuration so as to allow re-initialization
  DeleteContainer( fBdataLoc );

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  bool err = false;
  for( BdataLoc::TypeIter_t it = BdataLoc::fgBdataLocTypes.begin();
       !err && it != BdataLoc::fgBdataLocTypes.end(); ++it ) {
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
	Error( Here(here), "Incorrect number of parameters in database key %s. "
	       "Have %d, but must be a multiple of %d. Fix database.",
	       dbkey.Data(), nparams, loctype.nparams );
	err = true;
      }

      for( Int_t ip = 0; !err && ip < nparams; ip += loctype.nparams ) {
	BdataLoc* item = static_cast<BdataLoc*>( loctype.fTClass->New() );
	if( !item ) {
	  Error( Here(here), "Failed to create raw data type %s. "
		 "Should never happen. Call expert.", loctype.fTClass->GetName() );
	  err = true;
	  break;
	}
	err = item->Configure( params, ip );
	if( !err && loctype.optptr != 0 ) {
	  // Optional pointer to some type-specific data
	  err = item->OptionPtr( loctype.optptr );
	}
	if( err ) {
	  Int_t in = ip - (ip % loctype.nparams);
	  Error( Here(here), "Failed to configure raw data type %s item named "
		 "\"%s\"\n at parameter index = %d, value = \"%s\". "
		 "Fix database.", loctype.fTClass->GetName(),
		 (static_cast<TObjString*>(params->At(in)))->String().Data(),
		 ip,
		 (static_cast<TObjString*>(params->At(ip)))->String().Data() );
	}
      }
      
    }
    delete params;
  }

  fclose(file);
  if( err )
    return kInitError;

  // TODO: ReadDatabase stuff:

// // Set up the locations in raw data corresponding to variables of this class. 
// // Each element of a BdataLoc is one channel or word.  Since a channel 
// // may be a multihit device, the BdataLoc stores data in a vector.

//     if( !re_init ) {
//       fCrateLoc.clear();   
//       fWordLoc.clear(); 
//       BookHist();
//     }  

  //   ifstream decdatafile;

  //   const char* const here = "SetupDecData()";
  //   const char* name = GetDBFileName();

  //   TDatime date;
  //   if( run_time ) date = *run_time;
  //   vector<string> fnames = GetDBFileList( name, date, Here(here));
  //   // always look for 'decdata.map' in the current directory first.
  //   fnames.insert(fnames.begin(),string("decdata.map"));
  //   if( !fnames.empty() ) { //keep this test so we may comment out prev line
  //     vector<string>::iterator it = fnames.begin();
  //     do {
  // 	if( fDebug>0 ) {
  // 	  cout << "<" << IsA()->GetName() << "::" << Here(here)
  // 	       << ">: Opening database file " << *it;
  // 	}
  // 	decdatafile.clear();  // Forget previous failures before attempt
  // 	decdatafile.open((*it).c_str());

  // 	if( fDebug>0 ) {
  // 	  if( !decdatafile ) cout << " ... failed" << endl;
  // 	  else               cout << " ... ok" << endl;
  // 	}
  //     } while ( !decdatafile && ++it != fnames.end() );
  //   }
  //   if( fnames.empty() || !decdatafile ) {
  //     if( !re_init ) {
  // 	if( fDebug>0 )
  // 	  Warning( Here(here), "File db_%s.dat not found.\nAn example of this "
  // 		   "file should be in the examples directory.\nWill proceed "
  // 		   "with default mapping for THaDecData.", name );
  // 	return DefaultMap();
  //     } else {
  // 	if( fDebug>0 )
  // 	  Warning( Here(here), "File db_%s.dat not found for timestamp %s.\n"
  // 		   "Variable definitions unchanged from prior initialization.\n"
  // 		   "Update database to be sure you have valid data.",
  // 		   name, date.AsString() );
  // 	return retval;
  //     }
  //   }

  //   string sinput;
  //   const string comment = "#";
  //   while (getline(decdatafile, sinput)) {
  //     if( fDebug>3 )
  // 	Info( Here(here), "sinput = %s", sinput.c_str() );
  //     vector<string> strvect( vsplit(sinput) );
  //     if (strvect.size() < 5 || strvect[0] == comment) continue;
  //     bool found = false;
  //     RVarDef* pdef = vars;
  //     while( pdef->name && !found ) {
  // 	if( strvect[0] == pdef->name ) { 
  // 	  found = true; break;
  // 	}
  // 	++pdef;
  //     }
  //     Int_t slot(0), chan(0), skip(0);
  //     UInt_t header(0);
  //     Int_t crate = (Int_t)atoi(strvect[2].c_str());  // crate #
  //     bool is_slot = (strvect[1] == "crate");
  //     //FIXME: instantiate based on keyword -> class mapping
  //     // If BdataLoc is a ROOT class hierarchy, we can even dynamically
  //     // extend the list of supported data types with plugins
  //     BdataLoc* b;
  //     //FIXME: analyze strvect inside BdataLoc
  //     if( is_slot ) {  // Crate data ?
  // 	slot = (Int_t)atoi(strvect[3].c_str());
  // 	chan = (Int_t)atoi(strvect[4].c_str());
  // 	b = new BdataLoc(strvect[0].c_str(), crate, slot, chan);
  //     } else {         // Data is relative to a header
  // 	header = header_str_to_base16(strvect[3].c_str());
  // 	skip = (Int_t)atoi(strvect[4].c_str());
  // 	b = new BdataLoc(strvect[0].c_str(), crate, header, skip);
  //     }

  //     bool already_defined = false;
  //     if( re_init ) {
  // 	// When reinitializing, use simple logic, assuming things didn't 
  // 	// change much:
  // 	// - if the name exists, update it with the chan/slot read from the 
  // 	//   database for the new time
  // 	// - if the name is new, add it - leave it up to the user to decide
  // 	//   whether this is sensible
  // 	// - if a name disappears, things are ambiguous. Just leave it as 
  // 	//   it is, although something is probably wrong with the database
  // 	//
  // 	TString bname = b->GetName();
  // 	Iter_t p;
  // 	for( p = fWordLoc.begin();  p != fWordLoc.end(); ++p ) {
  // 	  if( bname == (*p)->GetName() ) {
  // 	    already_defined = true;
  // 	    break;
  // 	  }
  // 	}
  // 	if( !already_defined ) {
  // 	  for( p = fCrateLoc.begin(); p != fCrateLoc.end(); ++p ) {
  // 	    if( bname == (*p)->GetName() ) {
  // 	      already_defined = true;
  // 	      break;
  // 	    }
  // 	  }
  // 	}

  // 	if( already_defined ) {
  // 	  if ( **p != *b ) {
  // 	    if( fDebug>2 ) 
  // 	      Info( Here(here), 
  // 		    "Updating variable %s", (*p)->GetName() );
  // 	    //FIXME:  Ouch! If we change from slot to header or vice versa, we
  // 	    // must move the object from fCrateLoc to fWordLoc or vice versa...
  // 	    if( is_slot )
  // 	      (*p)->SetSlot( crate, slot, chan );
  // 	    else
  // 	      (*p)->SetHeader( crate, header, skip );
  // 	  } else {
  // 	    if( fDebug>2 )
  // 	      Info( Here(here),
  // 		    "Variable %s already defined and not changed",
  // 		    (*p)->GetName() );
  // 	  }
  // 	}
  //     }
      
  //     if( !already_defined ) {
  // 	if( found && fDebug>2 ) 
  // 	  Info( Here(here), "Defining standard variable %s", 
  // 		b->GetName() );
  // 	else if( !found && fDebug>2 ) 
  // 	  // !found is ok, but might be a typo error too, so I print to warn you.
  // 	  Info( Here(here), 
  // 		"New variable %s will become global", b->GetName() );

  // 	if( is_slot ) {
  // 	  fCrateLoc.push_back(b);
  // 	} else {
  // 	  fWordLoc.push_back(b);
  // 	}
	
  // 	if (!found) {
  // 	  // if not one of the pre-defined global variables, make a new global
  // 	  // variable for this BdataLoc
  // 	  DefineChannel(b,mode);
  // 	}
  //     }
  //   }
  // }

  return 0;
}


//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no detectors, we
  // skip the detector initialization.

  // Standard analysis object init, calls MakePrefix(), ReadDatabase()
  // and DefineVariables(), and Clear("I")
  return THaAnalysisObject::Init( run_time );
}

//_____________________________________________________________________________
//FIXME: junk this

// Int_t THaDecData::DefaultMap() {
// // Default setup of mapping of data in this class to locations in the raw data.
// // This is valid for a particular time.  If you have 'decdata.map' in your
// // pwd, the code would use that instead.  See /examples directory for an
// // example of decdata.map

// // ADCs that show data synch.
// //FIXME: note the beauty of the constructor overloading
//    Int_t crate = 1, slot = 25, chan = 16;   
//    fCrateLoc.push_back(new BdataLoc("synchadc1", crate, slot, chan));
//    fCrateLoc.push_back(new BdataLoc("synchadc2", 2, (Int_t) 24, 48));
//    fCrateLoc.push_back(new BdataLoc("synchadc3", 3, (Int_t) 22, 0));
//    fCrateLoc.push_back(new BdataLoc("synchadc4", 4, (Int_t) 17, 48));
//    fCrateLoc.push_back(new BdataLoc("synchadc14", 14, (Int_t) 1, 5));

// // Coincidence time, etc
//    fCrateLoc.push_back(new BdataLoc("ctimel", 4, (Int_t) 21, 48));
//    fCrateLoc.push_back(new BdataLoc("ctimer", 2, (Int_t) 16, 32));
//    fCrateLoc.push_back(new BdataLoc("pulser1", 3, (Int_t) 3, 7));

// // 100 kHz time stamp in roc14, at 2 words beyond header=0xfca56000
//    fCrateLoc.push_back(new BdataLoc("timestamp", 14, (UInt_t)0xfca56000, 2)); 

// // vxWorks time stamps
//    UInt_t header = 0xfabc0004;
//    Int_t ntoskip = 4;
//    crate = 1;
//    fWordLoc.push_back(new BdataLoc("timeroc1", crate, header, ntoskip));
//    fWordLoc.push_back(new BdataLoc("timeroc2", 2, (UInt_t)0xfabc0004, 4));
//    fWordLoc.push_back(new BdataLoc("timeroc3", 3, (UInt_t)0xfabc0004, 4));
//    fWordLoc.push_back(new BdataLoc("timeroc4", 4, (UInt_t)0xfabc0004, 4));
//    fWordLoc.push_back(new BdataLoc("timeroc14", 14, (UInt_t)0xfadcb0b4, 1));

// // RF time
//    fCrateLoc.push_back(new BdataLoc("rftime1", 2, (Int_t) 16, 50));
//    fCrateLoc.push_back(new BdataLoc("rftime2", 2, (Int_t) 16, 51));

// // EDTM pulser
//    fCrateLoc.push_back(new BdataLoc("edtpl", 3, (Int_t) 9, 81));
//    fCrateLoc.push_back(new BdataLoc("edtpr", 2, (Int_t) 12, 48));

// // Bit pattern for trigger definition

//    for (UInt_t i = 0; i < bits.GetNbits(); ++i) {
//      fCrateLoc.push_back(new BdataLoc(Form("bit%d",i+1), 3, (Int_t) 5, 64+i));
//    }

// // Anything else you want here...

//    return 0;
// }


//_____________________________________________________________________________
Int_t THaDecData::Decode(const THaEvData& evdata)
{
  // Extract the requested variables from the event data

  if( !IsOK() )
    return -1;

  Clear();

  // FIXME: the pain, THE PAIN
  // fix this by introducing another configration type "roclen"
  // lenroc12 = evdata.GetRocLength(12);
  // lenroc16 = evdata.GetRocLength(16);

  // For each raw data source registered in fBdataLoc, get the data 

  //TODO: accelerate search for multiple header words in same crate
  //- group WordLoc objects by crate
  //- for each crate with >1 WordLoc defs, make a MultiWordLoc
  //- MultiWordLoc uses faster search algo to scan crate buffer

  for( Iter_t p = fBdataLoc.begin(); p != fBdataLoc.end(); ++p) {
    BdataLoc *dataloc = *p;

    dataloc->Load( evdata );
  }
  
  evtype = evdata.GetEvType();   // CODA event type 

// debug 
//    Print();

  return 0;
}



//_____________________________________________________________________________
void THaDecData::Print( Option_t* ) const {
// Dump the data for purpose of debugging.
  cout << "Dump of THaDecData "<<endl;
  cout << "event pattern bits : ";
  // for (UInt_t i = 0; i < bits.GetNbits(); ++i) 
  //   cout << " "<<i<<" = "<< bits.TestBitNumber(i)<<"  | ";
  // cout << endl;
  cout << "event types,  CODA = "<<evtype<<"   bit pattern = "<<evtypebits<<endl;
  // cout << "synch adcs "<<"  "<<synchadc1<<"  "<<synchadc2<<"  ";
  // cout << synchadc3 <<"  "<<synchadc4<<"   "<<synchadc14<<endl;
  // cout <<" time stamps "<<timestamp<<"  "<<timeroc1<<"  "<<timeroc2<<"  ";
  // cout << timeroc3<<"  "<<timeroc4<<"  "<<timeroc14<<endl<<endl;
  // cout << "RF timing "<<rftime1<<"  "<<rftime2<<endl;
  // cout << "EDTM pulser "<<edtpl<<"  "<<edtpr<<endl;
  // cout << endl;
  // cout << "Crate-type variables: " << fCrateLoc.size() << endl;
  // cout << "Word-type variables: " << fWordLoc.size() << endl;
  // cout << "Histograms: " << hist.size() << endl;
}


//_____________________________________________________________________________
// void THaDecData::TrigBits(UInt_t ibit, BdataLoc *dataloc) {
// // Figure out which triggers got a hit.  These are multihit TDCs, so we
// // need to sort out which hit we want to take by applying cuts.

//   if( ibit >= kBitsPerByte*sizeof(UInt_t) ) return; //Limit of evtypebits
//   bits.ResetBitNumber(ibit);

//   //FIXME: MAKE THIS CUT CONFIGURABLE - SEPARATELY FOR EACH BIT!
//   const UInt_t cutlo = 0;
//   const UInt_t cuthi = 1500;
  
//   for (int ihit = 0; ihit < dataloc->NumHits(); ++ihit) {
//     if (dataloc->Get(ihit) > cutlo && dataloc->Get(ihit) < cuthi) {
//       bits.SetBitNumber(ibit);
//       evtypebits |= BIT(ibit);
//     }
//   }

// }
//_____________________________________________________________________________
ClassImp(THaDecData)

