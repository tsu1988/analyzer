//*-- Author :    Ole Hansen   26/04/2000


//////////////////////////////////////////////////////////////////////////
//
// THaVar
//
// A "global variable".  This is essentially a read-only pointer to the
// actual data, along with support for data types and arrays.  It can
// be used to retrieve data from anywhere in memory, e.g. analysis results
// of some detector object.
//
// THaVar objects are managed by the THaVarList.
//
// The standard way to define a variable is via the constructor, e.g.
//
//    Double_t x = 1.5;
//    THaVar v("x","variable x",x);
//
// Constructors and Set() determine the data type automatically.
// Supported types are
//    Double_t, Float_t, Long64_t, ULong64_t, Int_t, UInt_t,
//    Short_t, UShort_t, Char_t, Byte_t
//
// Arrays can be defined as follows:
//
//    Double_t* a = new Double_t[10];
//    THaVar* va = new THaVar("a[10]","array of doubles",a);
//
// One-dimensional variable-size arrays are also supported. The actual
// size must be contained in a variable of type Int_t.
// Example:
//
//    Double_t* a = new Double_t[20];
//    Int_t size;
//    GetValues( a, size );  //Fills a and sets size to number of data.
//    THaVar* va = new THaVar("a","var size array",a,&size);
//
// Any array size given in the name string is ignored (e.g. "a[20]"
// would have the same effect as "a"). The data are always interpreted
// as a one-dimensional array.
//
// Data are retrieved using GetValue(), which always returns Double_t.
// If access to the raw data is needed, one can use GetValuePointer()
// (with the appropriate caution).
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include "THaVar.h"
#include "TMethodCall.h"
#include "TObjArray.h"
#include "TClass.h"
#include "TError.h"
#include <cassert>
#include <vector>

using namespace std;

const Int_t    THaVar::kInvalidInt = -1;
const Double_t THaVar::kInvalid    = 1e38;

// NB: Must match definition order in VarDef.h
static struct VarTypeInfo_t {
  VarType      type;
  const char*  enum_name;  // name of enumeration constant to use for this type
  const char*  cpp_name;   // C++ type as understood by compiler
  size_t       size;       // size of underlying (innermost) data elements
}
  var_type_info[] = {
    { kDouble,   "kDouble",   "Double_t",    sizeof(Double_t)  },
    { kFloat,    "kFloat",    "Float_t",     sizeof(Float_t)   },
    { kLong,     "kLong",     "Long64_t",    sizeof(Long64_t)  },
    { kULong,    "kULong",    "ULong64_t",   sizeof(ULong64_t) },
    { kInt,      "kInt",      "Int_t",       sizeof(Int_t)     },
    { kUInt,     "kUInt",     "UInt_t",      sizeof(UInt_t)    },
    { kShort,    "kShort",    "Short_t",     sizeof(Short_t)   },
    { kUShort,   "kUShort",   "UShort_t",    sizeof(UShort_t)  },
    { kChar,     "kChar",     "Char_t",      sizeof(Char_t)    },
    { kByte,     "kByte",     "Byte_t",      sizeof(Byte_t)    },
    { kObject,   "kObject",   "TObject",     0                 },
    { kTString,  "kTString",  "TString",     sizeof(char)      },
    { kString,   "kString",   "string",      sizeof(char)      },
    { kIntV,     "kIntV",     "vector<int>",              sizeof(int)    },
    { kUIntV,    "kUIntV",    "vector<unsigned int>",     sizeof(unsigned int) },
    { kFloatV,   "kFloatV",   "vector<float>",            sizeof(float)  },
    { kDoubleV,  "kDoubleV",  "vector<double>",           sizeof(double) },
    { kIntM,     "kIntM",     "vector< vector<int> >",    sizeof(int)    },
    { kFloatM,   "kFloatM",   "vector< vector<float> >",  sizeof(float)  },
    { kDoubleM,  "kDoubleM",  "vector< vector<double> >", sizeof(double) },
    { kDoubleP,  "kDoubleP",  "Double_t*",   sizeof(Double_t)  },
    { kFloatP,   "kFloatP",   "Float_t*",    sizeof(Float_t)   },
    { kLongP,    "kLongP",    "Long64_t*",   sizeof(Long64_t)  },
    { kULongP,   "kULongP",   "ULong64_t*",  sizeof(ULong64_t) },
    { kIntP,     "kIntP",     "Int_t*",      sizeof(Int_t)     },
    { kUIntP,    "kUIntP",    "UInt_t*",     sizeof(UInt_t)    },
    { kShortP,   "kShortP",   "Short_t*",    sizeof(Short_t)   },
    { kUShortP,  "kUShortP",  "UShort_t*",   sizeof(UShort_t)  },
    { kCharP,    "kCharP",    "Char_t*",     sizeof(Char_t)    },
    { kByteP,    "kByteP",    "Byte_t*",     sizeof(Byte_t)    },
    { kObjectP,  "kObjectP",  "TObject*",    0                 },
    { kDouble2P, "kDouble2P", "Double_t**",  sizeof(Double_t)  },
    { kFloat2P,  "kFloat2P",  "Float_t**",   sizeof(Float_t)   },
    { kLong2P,   "kLong2P",   "Long64_t**",  sizeof(Long64_t)  },
    { kULong2P,  "kULong2P",  "ULong64_t**", sizeof(ULong64_t) },
    { kInt2P,    "kInt2P",    "Int_t**",     sizeof(Int_t)     },
    { kUInt2P,   "kUInt2P",   "UInt_t**",    sizeof(UInt_t)    },
    { kShort2P,  "kShort2P",  "Short_t**",   sizeof(Short_t)   },
    { kUShort2P, "kUShort2P", "UShort_t**",  sizeof(UShort_t)  },
    { kChar2P,   "kChar2P",   "Char_t**",    sizeof(Char_t)    },
    { kByte2P,   "kByte2P",   "Byte_t**",    sizeof(Byte_t)    },
    { kObject2P, "kObject2P", "TObject**",   0                 }
  };

// Static access function into var_type_info[] table above
//_____________________________________________________________________________
const char* THaVar::GetEnumName( VarType itype )
{
  // Return enumeration variable name of the given VarType

  assert( (size_t)itype < sizeof(var_type_info)/sizeof(VarTypeInfo_t) );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].enum_name;
}

//_____________________________________________________________________________
const char* THaVar::GetTypeName( VarType itype )
{
  // Return C++ name of the given VarType

  assert( (size_t)itype < sizeof(var_type_info)/sizeof(VarTypeInfo_t) );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].cpp_name;
}

//_____________________________________________________________________________
size_t THaVar::GetTypeSize( VarType itype )
{
  // Return size of the underlying (innermost) basic data type of each VarType.
  // Returns 0 for object types.

  assert( (size_t)itype < sizeof(var_type_info)/sizeof(VarTypeInfo_t) );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].size;
}

//_____________________________________________________________________________
THaVar::THaVar( const char* name, const char* desc, const void* obj,
		VarType type, Int_t offset, TMethodCall* method,
		const Int_t* count )
  : TNamed(name,desc), fParsedName(name), fObject(obj), fType(type),
    fCount(count), fOffset(offset), fMethod(method), fDim(0)
{
  // Generic constructor for any kind of THaVar (basic, object, function call)
  // The given type MUST match the type of the object pointed to!
  // Used by THaVarList::DefineByType.
  // User scripts should call the overloaded typed constructors.

  if( IsVector() ) {
    if( fCount != 0 ) {
      Warning( "THaVar", "Inconsistent constructor parameters for variable "
	       "\"%s\": vector must not have array size variable.", GetName() );
      fCount = 0;
    }
    if( fMethod != 0 ) {
      Warning( "THaVar", "Inconsistent constructor parameters for variable "
	       "\"%s\": vector must not have method call.", GetName() );
      delete fMethod; fMethod = 0;
    }
  }
  if( fCount != 0 && fMethod != 0 ) {
    Warning( "THaVar", "Inconsistent constructor parameters for variable "
	     "\"%s\": array size variable not allowed for method calls.",
	     GetName() );
    // Try to guess what the user intended
    if( fOffset != -1 )
      fCount = 0;
    else {
      delete fMethod; fMethod = 0;
    }
  }
}

//_____________________________________________________________________________
THaVar::THaVar( const THaVar& rhs ) :
  TNamed( rhs ), fParsedName(rhs.fParsedName), fValueP(rhs.fValueP),
  fType(rhs.fType), fCount(rhs.fCount), fOffset(rhs.fOffset),
  fMethod(rhs.fMethod), fDim(rhs.fDim)
{
  // Copy constructor

  // Make local copies of pointers
  if( fMethod ) fMethod = new TMethodCall( *rhs.fMethod );
}

//_____________________________________________________________________________
THaVar& THaVar::operator=( const THaVar& rhs )
{
  // Assignment operator. Must be explicit due to limitations of CINT.

  if( this != &rhs ) {
    TNamed::operator=(rhs);
    fValueP     = rhs.fValueP;  // pointer is not managed by us, so copy ok
    fParsedName = rhs.fParsedName;
    fType       = rhs.fType;
    fCount      = rhs.fCount;
    fOffset     = rhs.fOffset;
    // Make local copies of pointers
    delete fMethod;
    fMethod     = rhs.fMethod;
    if( fMethod ) fMethod  = new TMethodCall( *rhs.fMethod );
  }
  return *this;
}

//_____________________________________________________________________________
THaVar::~THaVar()
{
  // Destructor

  delete fMethod;
}

//_____________________________________________________________________________
Bool_t THaVar::HasSameSize( const THaVar& rhs ) const
{
  // Compare the size of this variable to that of 'rhs'.
  // Scalars always agree. Arrays agree if either they are of the same fixed
  // size, or their count variables are identical, or they belong to the
  // same object array. std::vectors never match since their sizes are, in
  // principle, independent of each other.

  Bool_t is_array = IsArray();
  if( is_array != rhs.IsArray())         // Must be same type
    return kFALSE;
  if( !is_array )                        // Scalars always agree
    return kTRUE;
  if( fCount )                           // Variable size arrays
    return ( fCount == rhs.fCount );
  if( IsVector() )                       // Vectors never match
    return kFALSE;
  if( fOffset != -1 )                    // Object arrays
    return ( fObject == rhs.fObject );
  return                                 // All other arrays
    ( fParsedName.GetNdim() == rhs.fParsedName.GetNdim() &&
      fParsedName.GetLen()  == rhs.fParsedName.GetLen() );
}

//_____________________________________________________________________________
Bool_t THaVar::HasSameSize( const THaVar* rhs ) const
{
  if( !rhs )
    return kFALSE;
  return HasSameSize( *rhs );
}

//_____________________________________________________________________________
Int_t THaVar::GetObjArrayLen() const
{
  // Interpret object as a TSeqCollection and return its current length

  if( !fObject )
    return kInvalidInt;

  const TObject* obj = static_cast<const TObject*>( fObject );
  if( !obj || !obj->IsA()->InheritsFrom( TSeqCollection::Class() ) )
    return THaVar::kInvalidInt;

  const TSeqCollection* c = static_cast<const TSeqCollection*>( obj );

  // Get actual array size
  if( c->IsA()->InheritsFrom( TObjArray::Class() ))
    // TObjArray is a special TSeqCollection for which GetSize() reports the
    // current capacity, not number of elements, so we need to use GetLast():
    return static_cast<const TObjArray*>(c)->GetLast()+1;

  return c->GetSize();
}

//_____________________________________________________________________________
Int_t THaVar::GetLen() const
{
  // Get number of elements of the variable

  if( fCount )
    return *fCount;

  if( fOffset != -1 )
    return GetObjArrayLen();

  if( IsVector() ) {
    // We cannot be sure that size() isn't specialized or type-dependent,
    // so to be safe, we cast the pointer to the exact type of vector
    switch( fType ) {
    case kIntV: {
      const vector<int>& vec = *static_cast< const vector<int>* >(fObject);
      return vec.size();
    }
    case kUIntV: {
      const vector<unsigned int>& vec =	*static_cast< const vector<unsigned int>* >(fObject);
      return vec.size();
    }
    case kFloatV: {
      const vector<float>& vec = *static_cast< const vector<float>* >(fObject);
      return vec.size();
    }
    case kDoubleV: {
      const vector<double>& vec = *static_cast< const vector<double>* >(fObject);
      return vec.size();
    }
    default:
      assert(false); // should never happen, bug in IsVector()
      return kInvalidInt;
    }
  }

  return fParsedName.GetLen();
}

//_____________________________________________________________________________
const Int_t* THaVar::GetDim() const
{
  // Return pointer to current array length. This is a legacy function for
  // compatibility with how THaArrayString handles multi-dimensional arrays.

  if( fCount )
    return fCount;
  if( fOffset != -1 || IsVector() ) {
    fDim = GetLen();
    return &fDim;
  }
  return fParsedName.GetDim();
}

//_____________________________________________________________________________
Double_t THaVar::GetValueAsDouble( Int_t i ) const
{
  // Retrieve current value of this global variable.
  // If the variable is an array/vector, return its i-th element.

#ifdef WITH_DEBUG
  if( i<0 || i>=GetLen() ) {
    Warning("GetValue()", "Whoa! Index out of range, variable %s, index %d",
	    GetName(), i );
    return kInvalid;
  }
#endif

  if( fMethod == 0 ) {

    const void* loc = GetDataPointer(i);
    if( !loc )
      return kInvalid;

    switch( fType ) {
    case kDouble:
    case kDoubleP:
    case kDouble2P:
    case kDoubleV:
	return *((Double_t*)loc);
    case kFloat:
    case kFloatP:
    case kFloat2P:
    case kFloatV:
      return *((Float_t*)loc);
    case kLong:
    case kLongP:
    case kLong2P:
      return *((Long64_t*)loc);
    case kULong:
    case kULongP:
    case kULong2P:
      return *((ULong64_t*)loc);
    case kInt:
    case kIntP:
    case kInt2P:
    case kIntV:
      return *((Int_t*)loc);
    case kUInt:
    case kUIntP:
    case kUInt2P:
    case kUIntV:
      return *((UInt_t*)loc);
    case kShort:
    case kShortP:
    case kShort2P:
      return *((Short_t*)loc);
    case kUShort:
    case kUShortP:
    case kUShort2P:
      return *((UShort_t*)loc);
    case kChar:
    case kCharP:
    case kChar2P:
      return *((Char_t*)loc);
    case kByte:
    case kByteP:
    case kByte2P:
      return *((Byte_t*)loc);
    case kObject:
    case kObjectP:
    case kObject2P:
      Error( "GetValue()", "Cannot get value from composite object, variable %s",
	     GetName() );
      return kInvalid;
    default:
      Error( "GetValue()", "Unsupported data type %s, variable %s",
	     GetEnumName(fType), GetName() );
      return kInvalid;
    }

  } else {
    // Method call
    union {
      Double_t dval;
      Int_t    ival;
    };

    Int_t nbytes = GetDataFromMethod(&dval,i);
    if( nbytes == 0 )
      return kInvalid;

    if( fType == kFloat || fType == kDouble )
      return dval;
    else
      return ival;
  }
  // Not reached
  return kInvalid;
}

//_____________________________________________________________________________
const void* THaVar::GetBasicDataPointer() const
{
  // For basic data only (i.e. not an object or a function call), return pointer
  // to actual data. In case of a PointerArray, this is not possible because the
  // data are not contiguous. In that case, one needs to use GetValuePointer and
  // dereference it on an element-by-element basis.

  if( !IsBasic() )
    return 0;

  if( fType >= kDouble && fType <= kByte )
    return fValueP;

  if( fType >= kDoubleP && fType <= kByteP )
    return *fValueDD;

  if( !IsArray() && fType >= kDouble2P && fType <= kByte2P )
    return **fValue3D;

  return 0;
}

//_____________________________________________________________________________
const void* THaVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data in memory, including to data in a non-contiguous
  // pointer array and data in objects stored in a TSeqCollection (TClonesArray)

  if( fObject == 0 || fMethod != 0 )
    return 0;

  assert( sizeof(ULong_t) == sizeof(void*) );
  assert( i>=0 && i < GetLen() );

  if( IsBasic() ) {
    const void* ptr = GetBasicDataPointer();
    if( !ptr )
      return 0;
    ULong_t loc = reinterpret_cast<ULong_t>(ptr) + i*GetTypeSize();
    return reinterpret_cast<const void*>(loc);
  }

  if( IsPointerArray() ) {
    const void** const *ptr = reinterpret_cast<const void** const *>(fValue3D);
    return (*ptr)[i];
  }

  if( IsVector() ) {
    if( GetLen() == 0 )
      return 0;
    switch( fType ) {
      case kIntV: {
	const vector<int>& vec = *static_cast< const vector<int>* >(fObject);
	return &vec[i];
      }
      case kUIntV: {
	const vector<unsigned int>& vec = *static_cast< const vector<unsigned int>* >(fObject);
	return &vec[i];
      }
      case kFloatV: {
	const vector<float>& vec = *static_cast< const vector<float>* >(fObject);
	return &vec[i];
      }
      case kDoubleV: {
	const vector<double>& vec = *static_cast< const vector<double>* >(fObject);
	return &vec[i];
      }
      default:
	return 0;
    }
  }

  if( fOffset != -1 ) {
    // Array: Get data from object in TSeqCollection

    void* obj = static_cast<const TSeqCollection*>( fObject )->At(i);
    assert(obj);
    if( !obj )
      return 0;

    // Compute location using the offset.
    ULong_t loc = reinterpret_cast<ULong_t>(obj) + fOffset;
    if( !loc || (fType >= kDoubleP && (*(void**)loc == 0 )) )
      return 0;

    return reinterpret_cast<const void*>(loc);
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaVar::GetDataFromMethod( void* buf, Int_t i ) const
{
  // Make a function call and return the result in buf (either Double_t or Int_t)
  // The return value is the number of bytes copied into the buffer (8 or 4)

  if( fObject == 0 || fMethod == 0 )
    return 0;

  void* obj;
  if( fOffset != -1 ) {
    // The method is part of an object in an array
    assert( i >= 0 && i<GetObjArrayLen() );

    obj = static_cast<const TSeqCollection*>(fObject)->At(i);
    assert(obj);
    if( !obj )
      return 0;

  } else {
    // No array. Make a function call on the object directly.
    // In this case, the index must be 0.
    assert( i == 0 );
    obj = const_cast<void*>(fObject);
  }

  if( fType != kDouble && fType != kFloat ) {
    // Integer data
    Long_t result;
    fMethod->Execute( obj, result );
    Int_t iresult = static_cast<Int_t>(result);
    memcpy( buf, &iresult, sizeof(iresult) );
    return sizeof(iresult);
  } else {
    // Floating-point data
    Double_t result;
    fMethod->Execute( obj, result );
    memcpy( buf, &result, sizeof(result) );
    return sizeof(result);
  }
}

//_____________________________________________________________________________
Int_t THaVar::Index( const THaArrayString& elem ) const
{
  // Return linear index into this array variable corresponding
  // to the array element described by 'elem'.
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  if( elem.IsError() ) return -1;
  if( !elem.IsArray() ) return 0;

  if( fCount ) {
    if( elem.GetNdim() != 1 ) return -2;
    return elem[0];
  }

  Int_t ndim = GetNdim();
  if( ndim != elem.GetNdim() ) return -2;

  const Int_t *adim = GetDim();

  Int_t index = elem[0];
  for( Int_t i = 0; i<ndim; i++ ) {
    if( elem[i] >= adim[i] ) return -1;
    if( i>0 )
      index = index*adim[i] + elem[i];
  }
  if( index >= GetLen() || index > kMaxInt ) return -1;
  return index;
}

//_____________________________________________________________________________
Int_t THaVar::Index( const char* s ) const
{
  // Return linear index into this array variable corresponding
  // to the array element described by the string 's'.
  // 's' must be either a single integer subscript (for a 1-d array)
  // or a comma-separated list of subscripts (for multi-dimensional arrays).
  //
  // NOTE: This method is less efficient than
  // THaVar::Index( THaArraySring& ) above because the string has
  // to be parsed first.
  //
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  size_t len = strlen(s);
  if( len == 0 ) return 0;

  char* str = new char[ len+4 ];
  str[0] = 't';
  str[1] = '[';
  str[len+2] = ']';
  str[len+3] = '\0';
  strncpy( str+2, s, len );
  THaArrayString elem(str);
  delete [] str;

  return Index( elem );
}

//_____________________________________________________________________________
void THaVar::Print(Option_t* option) const
{
  //Print a description of this variable

  TNamed::Print(option);

  if( strcmp( option, "FULL" )) return;

  cout << "(" << GetTypeName() << ")";
  if( IsArray() ) {
    Bool_t is_fixed = (IsBasic() && !IsVector() && fCount == 0);
    if( is_fixed )
      cout << "=";
    cout << "[";
    if( is_fixed ) {
      if( !fParsedName.IsArray() )
	Warning( "THaVar::Print", "Parsed name of fixed-size array is not "
		 "an array? Should never happen. Call expert." );
      fParsedName.Print("dimonly");
    }
    else
      cout << GetLen();
    cout << "]";
    for( int i=0; i<GetLen(); i++ ) {
      cout << "  " << GetValue(i);
    }
  } else
    cout << "  " << GetValue();

  cout << endl;
}

//_____________________________________________________________________________
void THaVar::SetName( const char* name )
{
  // Set the name of the variable

  TNamed::SetName( name );
  fParsedName = name;
}

//_____________________________________________________________________________
void THaVar::SetNameTitle( const char* name, const char* descript )
{
  // Set name and description of the variable

  TNamed::SetNameTitle( name, descript );
  fParsedName = name;
}

//_____________________________________________________________________________
ClassImp(THaVar)
