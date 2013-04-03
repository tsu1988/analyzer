#ifndef Podd_THaFileDB
#define Podd_THaFileDB

//////////////////////////////////////////////////////////////////////////
//
// THaFileDB
// 
// Preliminary version of the file-based database backend.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDB.h"
//#include <cstdio>

class THaFileDB : public THaDB {
 public:
  THaFileDB( const char* DB_DIR = 0 );
  virtual ~THaFileDB();
  
  //  LoadFile( );

  virtual Int_t Open();
  virtual Int_t Close();

  virtual Int_t Load( const DBRequest* request, const char* prefix="",
		      const TDatime& date = TDatime(), Int_t search = 0 );

  virtual Int_t LoadValue( const char* key, Double_t& value,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadValue( const char* key, Int_t& value,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadValue( const char* key, std::string& text,
			   const TDatime& date = TDatime() );
  virtual Int_t LoadValue( const char* key, TString& text,
			   const TDatime& date = TDatime() );

 protected:

  ClassDef(THaFileDB,0)   // Hall A-style file-based database backend
};

#endif

