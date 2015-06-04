//*-- Author :    Ole Hansen   03-Apr-13

//////////////////////////////////////////////////////////////////////////
//
// THaDB
//
// Generic database interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaDB.h"
#include "THaAnalysisObject.h"  // For Here()
#include "VarDef.h"
#include "TError.h"
#ifdef HAS_SSTREAM
 #include <sstream>
 #define ISSTREAM istringstream
#else
 #include <strstream>
 #define ISSTREAM istrstream
#endif

#include <cstdlib>
#include <cstring>
#include <cassert>

using namespace std;

typedef string::size_type ssiz_t;

//_____________________________________________________________________________
THaDB::THaDB( const char* URL ) : fURL(URL), fIsOpen(false), fDepth(0)
{
  // Constructor
}

//_____________________________________________________________________________
THaDB::~THaDB()
{
  // Destructor
}

//_____________________________________________________________________________
Int_t THaDB::Close()
{
  // Close the database connection

  fPrefix.Clear();
  fErrtxt.Clear();
  fIsOpen = false;

  return 0;
}

//_____________________________________________________________________________
Int_t THaDB::Open()
{
  // Open the database connection

  fIsOpen = true;

  return 0;
}

//___________________________________________________________________________
// Safe implementations of the various Load functions based on the
// Load( std::string& ) implemetation. This approach is straightforward
// but probably doesn't have the best performance

Int_t THaDB::LoadValue( const char* key, Double_t& dval, const TDatime& date )
{
  // Look up double-precision value

  string text;
  Int_t err = LoadValue( key, text, date );
  if( err == 0 ) {
    text += " ";
    ISSTREAM inp(text);
    inp >> dval;
    if( !inp )
      err = -1;
  }
  return err;
}

//_____________________________________________________________________________
Int_t THaDB::LoadValue( const char* key, Int_t& ival, const TDatime& date )
{
  // Look up integer value

  string text;
  Int_t err = LoadValue( key, text, date );
  if( err == 0 ) {
    text += " ";
    ISSTREAM inp(text);
    inp >> ival;
    if( !inp )
      err = -1;
  }
  return err;
}

//_____________________________________________________________________________
Int_t THaDB::LoadValue( const char* key, TString& text, const TDatime& date )
{
  // Look up a TString. This is a convenience function.

  string _text;
  Int_t err = LoadValue( key, _text, date );
  if( err == 0 ) {
#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
    text = _text;
#else
    text = _text.c_str();
#endif
  }
  return err;
}

//_____________________________________________________________________________
template <typename T> inline Int_t
_LoadArray( string& line, vector<T>& values, const TDatime& date )
{
  values.clear();
  line += " ";
  ISSTREAM inp(line);
  T val;
  while( 1 ) {
    inp >> val;
    if( inp.good() )
      values.push_back(val);
    else if( values.empty() )
      return -1;
    else
      break;
  }
  return 0;
}
//___________________________________________________________________________
template <typename T> inline Int_t
_LoadMatrix( const vector<T>& allvals, vector<vector<T> >& values,
	     UInt_t ncols, const TDatime& date )
{
  // Read a matrix of values of type T into a vector of vectors.
  // The matrix is square with ncols columns.

  if( ncols == 0 || (allvals.size() % ncols) != 0 ) {
    //TODO: throw exception with a message about the bad key:
    // errtxt = "key = "; errtxt += key;
    return -129;
  }
  typename vector<vector<T> >::size_type nrows = allvals.size()/ncols, irow;
  values.clear();
  values.reserve(nrows);
  vector<T> row;
  row.reserve(ncols);
  for( irow = 0; irow < nrows; ++irow ) {
    row.clear();
    for( typename vector<T>::size_type i=0; i<ncols; ++i ) {
      row.push_back( allvals.at(i+irow*ncols) );
    }
    values.push_back( row );
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaDB::LoadArray( const char* key, vector<string>& values,
			const TDatime& date )
{
  string line;
  Int_t err = LoadValue( key, line, date );
  if( err )
    return err;
  return _LoadArray( line, values, date );
}

//_____________________________________________________________________________
Int_t THaDB::LoadArray( const char* key, vector<Double_t>& values,
			const TDatime& date )
{
  string line;
  Int_t err = LoadValue( key, line, date );
  if( err )
    return err;
  return _LoadArray( line, values, date );
}

//_____________________________________________________________________________
Int_t THaDB::LoadArray( const char* key, vector<Float_t>& values,
			const TDatime& date )
{
  string line;
  Int_t err = LoadValue( key, line, date );
  if( err )
    return err;
  return _LoadArray( line, values, date );
}

//_____________________________________________________________________________
Int_t THaDB::LoadArray( const char* key, vector<Int_t>& values,
			const TDatime& date )
{
  string line;
  Int_t err = LoadValue( key, line, date );
  if( err )
    return err;
  return _LoadArray( line, values, date );
}

//___________________________________________________________________________
Int_t THaDB::LoadMatrix( const char* key, vector<vector<string> >& values,
			 UInt_t ncols, const TDatime& date )
{
  vector<string> allvals;
  Int_t err = LoadArray( key, allvals, date );
  if( err )
    return err;
  return _LoadMatrix( allvals, values, ncols, date );
}

//___________________________________________________________________________
Int_t THaDB::LoadMatrix( const char* key, vector<vector<Double_t> >& values,
			 UInt_t ncols, const TDatime& date )
{
  vector<Double_t> allvals;
  Int_t err = LoadArray( key, allvals, date );
  if( err )
    return err;
  return _LoadMatrix( allvals, values, ncols, date );
}

//___________________________________________________________________________
Int_t THaDB::LoadMatrix( const char* key, vector<vector<Float_t> >& values,
			 UInt_t ncols, const TDatime& date )
{
  vector<Float_t> allvals;
  Int_t err = LoadArray( key, allvals, date );
  if( err )
    return err;
  return _LoadMatrix( allvals, values, ncols, date );
}

//___________________________________________________________________________
Int_t THaDB::LoadMatrix( const char* key, vector<vector<Int_t> >& values,
			 UInt_t ncols, const TDatime& date )
{
  vector<Int_t> allvals;
  Int_t err = LoadArray( key, allvals, date );
  if( err )
    return err;
  return _LoadMatrix( allvals, values, ncols, date );
}

//_____________________________________________________________________________
static inline Int_t ChopPrefix( string& s )
{
  // Remove trailing level from prefix. Example "L.vdc." -> "L."
  // Return remaining number of dots, or zero if empty/invalid prefix

  ssiz_t len = s.size(), pos;
  Int_t ndot = 0;
  if( len<2 )
    goto null;
  pos = s.rfind('.',len-2);
  if( pos == string::npos )
    goto null;
  s.erase(pos+1);
  for( ssiz_t i = 0; i <= pos; i++ ) {
    if( s[i] == '.' )
      ndot++;
  }
  return ndot;

 null:
  s.clear();
  return 0;

}

//_____________________________________________________________________________
Int_t THaDB::Load( const DBRequest* req, const TDatime& date,
		   const char* prefix, Int_t search )
{
  // Load a list of parameters from the database file 'f' according to
  // the contents of the 'req' structure (see VarDef.h).

  // FIXME: use item->nelem to read arrays?

  const char* const here = "THaDB::Load";

  if( !req ) return -255;
  if( !prefix ) prefix = "";
  Int_t ret = 0;
  if( fDepth++ == 0 )
    fPrefix = prefix;

  const DBRequest* item = req;
  while( item->name ) {
    if( item->var ) {
      string keystr(prefix); keystr.append(item->name);
      const char* key = keystr.c_str();
      if( item->type == kDouble || item->type == kFloat ) {
	Double_t dval = 0.0;
	ret = LoadValue( key, dval, date );
	if( ret == 0 ) {
	  if( item->type == kDouble )
	    *((Double_t*)item->var) = dval;
	  else
	    *((Float_t*)item->var) = dval;
	}
      } else if( item->type >= kInt && item->type <= kByte ) {
	// Implies a certain order of definitions in VarType.h
	Int_t ival;
	ret = LoadValue( key, ival, date );
	if( ret == 0 ) {
	  switch( item->type ) {
	  case kInt:
	    *((Int_t*)item->var) = ival;
	    break;
	  case kUInt:
	    *((UInt_t*)item->var) = ival;
	    break;
	  case kShort:
	    *((Short_t*)item->var) = ival;
	    break;
	  case kUShort:
	    *((UShort_t*)item->var) = ival;
	    break;
	  case kChar:
	    *((Char_t*)item->var) = ival;
	    break;
	  case kByte:
	    *((Byte_t*)item->var) = ival;
	    break;
	  default:
	    goto badtype;
	  }
	}
      } else if( item->type == kString ) {
	ret = LoadValue( key, *((string*)item->var), date );
      } else if( item->type == kTString ) {
	ret = LoadValue( key, *((TString*)item->var) );
      } else if( item->type == kFloatV ) {
	ret = LoadArray( key, *((vector<Float_t>*)item->var), date );
      } else if( item->type == kDoubleV ) {
	ret = LoadArray( key, *((vector<Double_t>*)item->var), date );
      } else if( item->type == kIntV ) {
	ret = LoadArray( key, *((vector<Int_t>*)item->var), date );
      } else if( item->type == kFloatM ) {
	ret = LoadMatrix( key, *((vector<vector<Float_t> >*)item->var),
			  item->nelem );
      } else if( item->type == kDoubleM ) {
	ret = LoadMatrix( key, *((vector<vector<Double_t> >*)item->var),
			  item->nelem );
      } else if( item->type == kIntM ) {
	ret = LoadMatrix( key, *((vector<vector<Int_t> >*)item->var),
			  item->nelem );
      } else {
      badtype:
	const char* type_name;
	if( item->type >= kDouble && item->type <= kObject2P )
	  type_name = var_type_name[item->type];
	else
	  type_name = Form("(#%d)", item->type );
	::Error( ::Here(here, fPrefix),
		 "Key \"%s\": Reading of data type \"%s\" not implemented",
		 type_name, key );
	ret = -2;
	break;
      }

      if( ret == 0 ) {  // Key found -> next item
	goto nextitem;
      } else if( ret > 0 ) {  // Key not found
	// If searching specified, either for this key or globally, retry
	// finding the key at the next level up along the name tree. Name
	// tree levels are defined by dots (".") in the prefix. The top
	// level is 1 (where prefix = "").
	// Example: key = "nw", prefix = "L.vdc.u1", search = 1, then
	// search for:  "L.vdc.u1.nw" -> "L.vdc.nw" -> "L.nw" -> "nw"
	//
	// Negative values of 'search' mean search up relative to the
	// current level by at most abs(search) steps, or up to top level.
	// Example: key = "nw", prefix = "L.vdc.u1", search = -1, then
	// search for:  "L.vdc.u1.nw" -> "L.vdc.nw"

	// per-item search level overrides global one
	Int_t newsearch = (item->search != 0) ? item->search : search;
	if( newsearch != 0 && *prefix ) {
	  string newprefix(prefix);
	  Int_t newlevel = ChopPrefix(newprefix) + 1;
	  if( newsearch < 0 || newlevel >= newsearch ) {
	    DBRequest newreq[2];
	    newreq[0] = *item;
	    memset( newreq+1, 0, sizeof(DBRequest) );
	    newreq->search = 0;
	    if( newsearch < 0 )
	      newsearch++;
	    ret = Load( newreq, date, newprefix.c_str(), newsearch );
	    // If error, quit here. Error message printed at lowest level.
	    if( ret != 0 )
	      break;
	    goto nextitem;  // Key found and ok
	  }
	}
	if( item->optional )
	  ret = 0;
	else {
	  if( item->descript ) {
	    ::Error( ::Here(here,fPrefix),
		     "Required key \"%s\" (%s) missing in the database.",
		     key, item->descript );
	  } else {
	    ::Error( ::Here(here,fPrefix),
		     "Required key \"%s\" missing in the database.", key );
	  }
	  // For missing keys, the return code is the index into the request
	  // array + 1. In this way the caller knows which key is missing.
	  ret = 1+(item-req);
	  break;
	}
// FIXME: handle ret = -1 -> conversion error
// FIXME: use exceptions instead?
      } else if( ret == -128 ) {  // Line too long
	::Error( ::Here(here,fPrefix),
		 "Text line too long. Fix the database!\n\"%s...\"",
		 fErrtxt.Data() );
	break;
      } else if( ret == -129 ) {  // Matrix ncols mismatch
	::Error( ::Here(here,fPrefix),
		 "Number of matrix elements not evenly divisible by requested "
		 "number of columns. Fix the database!\n\"%s...\"",
		 fErrtxt.Data() );
	break;
      } else {  // other ret < 0: unexpected zero pointer etc.
	::Error( ::Here(here,fPrefix),
		 "Program error when trying to read database key \"%s\". "
		 "CALL EXPERT!", key );
	break;
      }
    }
  nextitem:
    item++;
  }
  if( --fDepth == 0 )
    fPrefix.Clear();

  return ret;
}

//_____________________________________________________________________________
ClassImp(THaDB)
