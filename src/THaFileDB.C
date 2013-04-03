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
#include "VarDef.h"

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
Int_t THaFileDB::Open() 
{
  // Open the database connection

  return 0;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, Double_t& value,
			    const TDatime& date )
{

  return 0;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, Int_t& value,
			    const TDatime& date )
{

  return 0;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, std::string& text,
			    const TDatime& date )
{

  return 0;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, TString& text,
			    const TDatime& date )
{

  return 0;
}

//_____________________________________________________________________________
Int_t THaFileDB::Load( const DBRequest* request, const char* prefix,
		       const TDatime& date, Int_t search )
{


  return 0;
}

//_____________________________________________________________________________
ClassImp(THaFileDB)
