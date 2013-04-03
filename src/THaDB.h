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

struct DBRequest;

class THaDB {
 public:
  THaDB( const char* URL );
  virtual ~THaDB();
  
  virtual Int_t Open();
  virtual Int_t Close();

  virtual Int_t Load( const DBRequest* request, const char* prefix="",
		      const TDatime& date = TDatime(), Int_t search = 0 );

  // Load functions
  virtual Int_t LoadValue( const char* key, std::string& text,
			   const TDatime& date = TDatime() ) = 0;
  virtual Int_t LoadValue( const char* key, Double_t& dval,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadValue( const char* key, Int_t& ival,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadValue( const char* key, TString& text,
			   const TDatime& date = TDatime() );

  // Arrays/vectors
  virtual Int_t LoadArray( const char* key, std::vector<std::string>& values,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadArray( const char* key, std::vector<Double_t>& values,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadArray( const char* key, std::vector<Int_t>& values,
			   const TDatime& date = TDatime() );

  // Matrices
  virtual Int_t LoadMatrix( const char* key,
			    std::vector<std::vector<Double_t> >& values,
			    UInt_t ncols, const TDatime& date = TDatime() );
  virtual Int_t LoadMatrix( const char* key,
			    std::vector<std::vector<Int_t> >& values,
			    UInt_t ncols, const TDatime& date = TDatime() );
  virtual Int_t LoadMatrix( const char* key,
			    std::vector<std::vector<std::string> >& values,
			    UInt_t ncols, const TDatime& date = TDatime() );

 protected:

  TString  fURL;      // URL for database connection
  Bool_t   fIsOpen;   // Open() called successfully

  ClassDef(THaDB,0)   // Generic database interface
};

#endif

