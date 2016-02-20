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
#include <cstring> // for memcpy
#include <cassert>

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

  fData = 0;
  if( IsFloat() ) {
    // Floating-point data
    Double_t result;
    fMethod->Execute( pobj, result );
    switch( fType ) {
    case kDouble:
      fData = result;
      break;
    case kFloat:
      {
	// This must fit without overflow, otherwise the interpreter lied to us
	// when it gave us the function return type
	Float_t resultF = result;
	memcpy( &fData, &resultF, sizeof(resultF) );
      }
      break;
    default:
      assert(false);  // misconstructed object (error in THaVarList::DefineByRTTI)
    }
    return &fData;
  }
  else {
    // Integer data
    Long_t result;  // TMethodCall really wants Long_t, not Long64_t
    fMethod->Execute( pobj, result );
    switch( fType ) {
    case kLong:
    case kULong:
      memcpy( &fData, &result, sizeof(result) );
      break;
    case kInt:
    case kUInt:
      {
	Int_t resultI = result;
	memcpy( &fData, &resultI, sizeof(resultI) );
      }
      break;
    case kShort:
    case kUShort:
      {
	Short_t resultS = result;
	memcpy( &fData, &resultS, sizeof(resultS) );
      }
      break;
    case kChar:
    case kUChar:
      {
	Char_t resultC = result;
	memcpy( &fData, &resultC, sizeof(resultC) );
      }
      break;
    default:
      assert(false);  // misconstructed object (error in THaVarList::DefineByRTTI)
    }
    return &fData;
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
