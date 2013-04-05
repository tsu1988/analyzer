//*-- Author :    Ole Hansen   03-Apr-13

//////////////////////////////////////////////////////////////////////////
//
// THaFileDB
//
// Backend for Hall A-style database files, stored in time-stamped
// directories.
//
//////////////////////////////////////////////////////////////////////////

#include "THaFileDB.h"
#include <cassert>

using namespace std;

//_____________________________________________________________________________
THaFileDB::THaFileDB( const char* DB_DIR ) : THaDB(DB_DIR)
{
  // Constructor
}

//_____________________________________________________________________________
THaFileDB::~THaFileDB()
{
  // Destructor

  Close();
}

//_____________________________________________________________________________
Int_t THaFileDB::Close() 
{
  // Close the database connection

  fKeys.clear();
  fValues.clear();
  fTimeRanges.clear();
  
  return THaDB::Close();
}

//_____________________________________________________________________________
Int_t THaFileDB::LookupKey( const char* key, string& text, time_t timet )
{
  // Look up the value for the given key and time as a string.
  // Returns 0 on success, 1 if key not found, <0 if unexpected error.

  typedef ValueMap_t::iterator valiter_t;
  typedef pair<valiter_t,valiter_t> value_range_t;

  if( !key || !*key ) return 1;

  // Map the key string to its ID
  StringMap_t::iterator ik = fKeys.find( string(key) );
  if( ik == fKeys.end() )  // No such key
    return 1;

  // Find the value corresponding to this key ID
  value_range_t range = fValues.equal_range( ik->second );
  assert( range.first != fValues.end() );  // Value must exist

  // Check if any of the value's time ranges matches the requested time
  for( valiter_t it = range.first; it != range.second; ++it ) {
    const Value_t& theValue = (*it).second;
    TimeRangeMap_t::iterator ir = fTimeRanges.find( theValue.range_id );
    assert( ir != fTimeRanges.end() );
    // The time ranges are unique by construction, so we only need to check
    // for a single match
    const TimeRange_t& theRange = (*ir).second;
    if( theRange.start_time <= timet && timet < theRange.end_time ) {
      text = theValue.value;
      return 0;
    }
  }
  return 1;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, string& text, const TDatime& date )
{
  // Retrieve the value for the given key as a string. Converts the given date
  // to time_t, using the current locale, then calls LookupKey.
  // The TDatime object must be given in the local time of the current machine.

  return LookupKey( key, text, date.Convert() );
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadFile( FILE* file, const TDatime& date )
{

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaFileDB)
