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
#include <map>
#include <set>
#include <cstdio>
#include <ctime>

class THaFileDB : public THaDB {
 public:
  THaFileDB( const char* DB_DIR = 0 );
  virtual ~THaFileDB();
  
  // Prevent hiding of the overloaded LoadValue instances
  using THaDB::LoadValue;

  virtual Int_t Close();
  virtual Int_t LoadValue( const char* key, std::string& text,
			   const TDatime& date = TDatime() );

  Int_t LookupKey( const char* key, std::string& text, time_t timestamp );

  // Support functions for THaAnalyzer::LoadDB interface.
  // Will be deprecated in a future release.

  Int_t LoadFile( FILE* file, const TDatime& date );

 protected:


  struct Value_t {
    UInt_t       range_id;
    std::string  value;
  };
  struct TimeRange_t {
    time_t       start_time;
    time_t       end_time;
  };

  typedef std::map<std::string,UInt_t>   StringMap_t;
  typedef std::multimap<UInt_t,Value_t>  ValueMap_t;
  typedef std::map<UInt_t,TimeRange_t>   TimeRangeMap_t;

  StringMap_t    fKeys;         // Unique keys -> key IDs (1-to-1)
  ValueMap_t     fValues;       // key IDs -> ranges/values (1-to-many)
  TimeRangeMap_t fTimeRanges;   // Range IDs -> time periods (1-to-1)

  ClassDef(THaFileDB,0)   // Hall A-style file-based database backend
};

#endif

