#ifndef ROOT_THaArrayString
#define ROOT_THaArrayString

//////////////////////////////////////////////////////////////////////////
//
// THaArrayString
//
//////////////////////////////////////////////////////////////////////////

#include "TString.h"

class THaArrayString {
  
public:
  enum EStatus { kOK, kBadsyntax, kIllegalchars, kToolarge,  kToolong, 
		 kNotinit };

  THaArrayString() : fNdim(0), fDims(0), fStatus(kNotinit) {}
  THaArrayString( const char* string )
    : fName(string), fNdim(0), fDims(0), fStatus(kNotinit) { Parse(string); }
  THaArrayString( const THaArrayString& );
  THaArrayString& operator=( const THaArrayString& );
  THaArrayString& operator=( const char* rhs ) { Parse( rhs ); return *this; }
  bool operator==( const THaArrayString& rhs ) const;
  bool operator!=( const THaArrayString& rhs ) const  { return !(*this ==rhs ); }
  virtual ~THaArrayString();

  operator const char*() const { return fName.Data(); }

  Int_t           GetDim( Int_t i ) const;
  Int_t           GetLen()  const;
  const char*     GetName() const { return fName.Data(); }
  Int_t           GetNdim() const { return fNdim; }
  Bool_t          HasEqualDims( const THaArrayString& rhs ) const;
  ULong_t         Hash()    const { return fName.Hash(); }
  Bool_t          IsArray() const { return (fNdim > 0); }
  Bool_t          IsError() const { return (fStatus != kOK); }
  virtual Int_t   Parse( const char* string="" );
  virtual void    Print( Option_t* opt="" ) const;
  EStatus         Status()  const { return fStatus; }
  const TString&  String()  const { return fName; }

protected:
  TString  fName;            //Variable name
  Int_t    fNdim;            //Number of array dimensions (0=scalar)
  union {
    Int_t  fDim;             //1-d array dimension (0 for scalar)
    Int_t* fDims;            //>1-d dimensions
  };
  EStatus  fStatus;          //Status of Parse()

  void     PrintDims() const;

  ClassDef(THaArrayString,0) //Parser for variable names with support for arrays
};

//__________ inlines __________________________________________________________
inline
Int_t THaArrayString::GetDim( Int_t i ) const
{
  // Return number of elements for the i-th dimension (0...fNdim-1), or 0
  // if i is out of range. These are exactly the numbers found within square
  // brackets in the parsed string that was given to initialize this object.

  if( i<0 || i>fNdim-1 ) return 0;
  return (fNdim==1) ? fDim : fDims[i];
}

//_____________________________________________________________________________
inline
Int_t THaArrayString::GetLen() const
{
  // Return length of the array (product of all dimensions)

  if( fNdim == 0 )
    return 1;
  if( fNdim == 1 )
    return fDim;
  Int_t len = fDims[0];
  for( Int_t i=1; i<fNdim; ++i )
    len *= fDims[i];
  return len;
}

//_____________________________________________________________________________
inline
Bool_t THaArrayString::HasEqualDims( const THaArrayString& rhs ) const
{
  // Compare dimensions (fNdim and all fDims) of two THaArrayStrings

  if( fNdim != rhs.fNdim ) return kFALSE;
  if( fNdim == 0 )         return kTRUE;
  for( Int_t i=0; i<fNdim; ++i ) {
    if( GetDim(i) != rhs.GetDim(i) )
      return kFALSE;
  }
  return kTRUE;
}

//_____________________________________________________________________________
inline
bool THaArrayString::operator==( const THaArrayString& rhs ) const
{
  if( fName != rhs.fName ) return kFALSE;
  return HasEqualDims( rhs );
}


#endif

