//*-- Author :    Ole Hansen   18/2/2016

//////////////////////////////////////////////////////////////////////////
//
// MethodVar
//
// A "global variable" referencing a class member function call
//
//////////////////////////////////////////////////////////////////////////

#include "MethodVar.h"
#include "THaVar.h"
#include "TError.h"
#include "TMethodCall.h"
#include <cassert>

#define kInvalid     THaVar::kInvalid
#define kInvalidInt  THaVar::kInvalidInt

using namespace std;

namespace Podd {

//_____________________________________________________________________________
MethodVar::MethodVar( THaVar* pvar, const void* addr,
		      VarType type, TMethodCall* method )
  : Variable(pvar,addr,type), fMethod(method), fData(0)
{
  // Constructor
  assert( fMethod );
}

//_____________________________________________________________________________
const void* MethodVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data in objects stored in a TSeqCollection (TClonesArray)

  const char* const here = "GetDataPointer()";

  assert( fValueP );
  assert( fMethod );

  if( i != 0 ) {
    fSelf->Error( here, "Index out of range, variable %s, index %d", GetName(), i );
    return 0;
  }

  return GetDataPointer(fValueP);
}

//_____________________________________________________________________________
const void* MethodVar::GetDataPointer( const void* obj ) const
{
  // Make the method call on the object pointed to by 'obj'

  void* pobj = const_cast<void*>(obj);  // TMethodCall wants a non-const object...

  if( IsFloat() ) {
    // Floating-point data
    fMethod->Execute( pobj, fData );
    return &fData;
  } else {
    // Integer data
    Long_t result;  // TMethodCall really wants Long_t, not Long64_t
    fMethod->Execute( pobj, result );
    fDataInt = result;
    return &fDataInt;
  }
  // Not reached
  return 0;
}

//_____________________________________________________________________________
Bool_t MethodVar::IsBasic() const
{
  // Data are basic (POD variable or array)

  return kFALSE;
}

//_____________________________________________________________________________

}// namespace Podd

ClassImp(Podd::MethodVar)
