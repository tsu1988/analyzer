//*-- Author :    Ole Hansen   24-Oct-08

//////////////////////////////////////////////////////////////////////////
//
// THaEventTypeHandler
//
// Abstract base class for an event type handler used by THaAnalyzer
//
//////////////////////////////////////////////////////////////////////////

#include "THaEventTypeHandler.h"
#include "THaGlobals.h"
#include <stdexcept>
#include <string>

using namespace std;

//_____________________________________________________________________________
THaEventTypeHandler::THaEventTypeHandler()
{
  // Normal constructor.

  // TODO: set fSortPos to "order created" by default
}

//_____________________________________________________________________________
THaEventTypeHandler::~THaEventTypeHandler()
{
  // Destructor

}

//_____________________________________________________________________________
Int_t THaEventTypeHandler::Begin()
{
  return 0;
}

//_____________________________________________________________________________
void THaEventTypeHandler::Clear( Option_t* )
{
  fCount = 0;
}

//_____________________________________________________________________________
void THaEventTypeHandler::Close()
{
}

//_____________________________________________________________________________
Int_t THaEventTypeHandler::Compare( const TObject* obj ) const
{
  // Compare function for sorting TList of THaEventType handlers by priority
  // (smallest fSortPos first).

  if( this == obj ) return  0;

  const THaEventTypeHandler* rhs =
    dynamic_cast<const THaEventTypeHandler*>( obj );
  if( !rhs ) {
    string msg("THaEventTypeHandler::Compare(obj) invalid class of obj");
    throw std::invalid_argument(msg);
  }

  if( fSortPos < rhs->fSortPos ) return -1;
  if( fSortPos > rhs->fSortPos ) return  1;
  return 0;
}

//_____________________________________________________________________________
Int_t THaEventTypeHandler::Init( const TDatime& /* date */ ,
				 const TList*   /* module_list */ )
{
  // Initialization of event type handler module. Called from THaAnalyzer
  // after the output file has been opened, the run object as been initialized,
  // and the list of analysis modules is set up.
  //
  // The extracted run time and the list of modules are passed into this
  // routine as parameters "date" and "module_list"
  //
  // The default version does nothing. Derived classes will almost always want
  // to override this.

  return 0;
}

//_____________________________________________________________________________
Int_t THaEventTypeHandler::InitLevel2()
{
  // Level 2 initialization. Called from THaAnalyzer after global variables
  // are set up, cuts, tests, and histograms are defined, and the output class
  // has been initialized.
  //
  // The default version does nothing. Some derived classes (typically ones
  // doing complex analysis) may need to override this.

  return 0;
}

//_____________________________________________________________________________
Int_t THaEventTypeHandler::End()
{
  return 0;
}

//_____________________________________________________________________________
void THaEventTypeHandler::Print( Option_t* ) const
{
  fCount.Print();
}

//_____________________________________________________________________________
void THaEventTypeHandler::SetSortPos( Int_t i )
{
  fSortPos = i;
}


//_____________________________________________________________________________
ClassImp(THaEventTypeHandler)
