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
  
  using THaDB::LoadValue;

  //  LoadFile( );

  virtual Int_t Close();

  virtual Int_t LoadValue( const char* key, std::string& text,
			   const TDatime& date = TDatime() );

 protected:

  ClassDef(THaFileDB,0)   // Hall A-style file-based database backend
};

#endif

