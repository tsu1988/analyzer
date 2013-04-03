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
}

//_____________________________________________________________________________
Int_t THaFileDB::Close() 
{
  // Close the database connection

  return 0;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, string& text, const TDatime& date )
{

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaFileDB)
