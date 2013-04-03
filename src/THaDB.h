#ifndef Podd_THaDB
#define Podd_THaDB

//////////////////////////////////////////////////////////////////////////
//
// THaDB
// 
//////////////////////////////////////////////////////////////////////////

#include "TDatime.h"
#include "TString.h"
#include <vector>
#include <string>
#ifdef HAS_SSTREAM
 #include <sstream>
 #define ISSTREAM istringstream
#else
 #include <strstream>
 #define ISSTREAM istrstream
#endif

struct DBRequest;

class THaDB {
 public:
  THaDB( const char* URL );
  virtual ~THaDB();
  
  virtual Int_t Open();
  virtual Int_t Close();

  virtual Int_t Load( const DBRequest* request, const char* prefix="",
		      const TDatime& date = TDatime(), Int_t search = 0 ) = 0;

  virtual Int_t LoadValue( const char* key, Double_t& value,
			   const TDatime& date = TDatime() ) = 0;
  virtual Int_t LoadValue( const char* key, Int_t& value,
			   const TDatime& date = TDatime() ) = 0;
  virtual Int_t LoadValue( const char* key, std::string& text,
			   const TDatime& date = TDatime() ) = 0;
  virtual Int_t LoadValue( const char* key, TString& text,
			   const TDatime& date = TDatime() ) = 0;

  //___________________________________________________________________________
  template <typename T>
  Int_t LoadArray( const char* key, std::vector<T>& values,
		   const TDatime& date = TDatime() )
  {
    using namespace std;
    string text;
    Int_t err = LoadValue( key, text, date );
    if( err )
      return err;
    values.clear();
    text += " ";
    ISSTREAM inp(text);
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
  template <typename T>
  Int_t LoadMatrix( const char* key,
		    std::vector<std::vector<T> >& values, UInt_t ncols,
		    const TDatime& date = TDatime() )
  {
    // Read a matrix of values of type T into a vector of vectors.
    // The matrix is square with ncols columns.

    using namespace std;
    vector<T> allvals;
    Int_t err = LoadArray( key, allvals, date );
    if( err )
      return err;
    if( ncols == 0 || (allvals.size() % ncols) != 0 ) {
      //TODO: throw exception
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
  //___________________________________________________________________________

 protected:

  TString  fURL;      // URL for database connection
  Bool_t   fIsOpen;   // Open() called successfully

  ClassDef(THaDB,0)   // Generic database interface
};

#endif

