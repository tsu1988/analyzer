//*-- Author :    Ole Hansen   03-Apr-13

//////////////////////////////////////////////////////////////////////////
//
// THaDB
//
// Generic database interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaDB.h"
#include "VarDef.h"
#ifdef HAS_SSTREAM
 #include <sstream>
 #define ISSTREAM istringstream
#else
 #include <strstream>
 #define ISSTREAM istrstream
#endif

#include <cstdlib>

using namespace std;

//_____________________________________________________________________________
THaDB::THaDB( const char* URL ) : fURL(URL), fIsOpen(false)
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
Int_t THaDB::Load( const DBRequest* request, const char* prefix,
		   const TDatime& date, Int_t search )
{


  return 0;
}

//_____________________________________________________________________________
ClassImp(THaDB)
