//*-- Author :    Ole Hansen   03-Apr-13

//////////////////////////////////////////////////////////////////////////
//
// THaDB
//
// Generic database interface
//
//////////////////////////////////////////////////////////////////////////

#include "THaDB.h"

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

//_____________________________________________________________________________
ClassImp(THaDB)
