//////////////////////////////////////////////////////////////////////////
//
// Output::HistogramAxis
//
//////////////////////////////////////////////////////////////////////////

#include "HistogramAxis.h"
#include "THaFormula.h"
#include "THaVar.h"
#include "THaVarList.h"
#include <string>
#include <cassert>
#include <sstream>

using namespace std;

namespace Output {

//___________________________________________________________________________
Bool_t HistogramAxis::IsEye( const string& expr, Int_t& offset )
{
  // Test 'expr' for "eye" expression "[I]" or "[i]", possibly with an
  // offset, e.g. "[I+1]". Returns true if matching. The offset is
  // extracted to 'offset'.

  string::size_type len = expr.size();
  if( len >= 3 && expr[0] == '[' && (expr[1] == 'I' || expr[1] == 'i') &&
      expr[len-1] == ']' ) {
    // If len == 3, we have "[I]" or "[i]"
    if( len == 3 ) {
      offset = 0;
      return true;
    }
    // Something follows the initial "[I". Try converting it to an integer.
    // No whitespace allowed
    if( expr[2] == '+' || expr[2] == '-' ) {
      istringstream iss(expr.substr(2,len-3));
      Int_t i;
      iss >> i;
      if( iss.eof() ) { // else trailing garbage
	offset = i;
	return true;
      }
    }
  }
  return false;
}

//=============================================================================
class AxisImpl {
public:
  AxisImpl() {}
  virtual ~AxisImpl() {}

  virtual AxisImpl*   Clone()      const = 0;
  virtual Double_t    GetData( Int_t i ) const = 0;
  virtual const char* GetName()    const = 0;
  virtual Int_t       GetSize()    const = 0;
  virtual Bool_t      IsScalar()   const { return !IsArray(); }
  virtual Bool_t      IsArray()    const = 0;
  virtual Bool_t      IsVarArray() const = 0;
  virtual void        ReAttach() {}
};

//___________________________________________________________________________
// Dummy data source. Provides no data
// class DummyAxis : public AxisImpl {
// public:
//   DummyAxis() {}

//   virtual Double_t GetData( Int_t i ) const
//   {
//     assert( i>=0 && i<GetSize() );
//     return 0.0;
//   }
//   virtual const char* GetName()    const { return ""; }
//   virtual Int_t       GetSize()    const { return 0; }
//   virtual Bool_t      IsArray()    const { return false; }
//   virtual Bool_t      IsVarArray() const { return false; }
// };

//___________________________________________________________________________
// "Eye" axis. Always returns instance index
class EyeAxis : public AxisImpl {
public:
  EyeAxis( Int_t offset = 0 ) : fOffset(offset) {}

  virtual EyeAxis*    Clone()      const { return new EyeAxis(fOffset); }
  virtual Double_t    GetData( Int_t i ) const { return i+fOffset; }
  virtual const char* GetName()    const { return "eye"; }
  virtual Int_t       GetSize()    const { return -1; }
  virtual Bool_t      IsArray()    const { return false; }
  virtual Bool_t      IsVarArray() const { return false; }

protected:
  Int_t fOffset;
};

//___________________________________________________________________________
// External formula. Expected to be managed by caller
class FormulaAxis : public AxisImpl {
public:
  FormulaAxis( THaFormula* theFormula )
    : fFormula(theFormula) { assert(fFormula); }

  virtual FormulaAxis* Clone() const { return new FormulaAxis(fFormula); }
  virtual Double_t GetData( Int_t i ) const
  {
    assert( i>=0 && i<GetSize() );
    Double_t x = fFormula->EvalInstance(i);
    assert( !fFormula->IsInvalid() );
    return x;
  }
  virtual const char* GetName()    const { return fFormula->GetName(); }
  virtual Int_t       GetSize()    const { return fFormula->GetNdata(); }
  virtual Bool_t      IsArray()    const { return fFormula->IsArray(); }
  virtual Bool_t      IsVarArray() const { return fFormula->IsVarArray(); }
  virtual void        ReAttach()         { fFormula->Compile(); }

protected:
  THaFormula* fFormula;
};

//___________________________________________________________________________
// Internal formula. Compiled and managed locally
class InternalFormulaAxis : public FormulaAxis {
public:
  InternalFormulaAxis( const string& name, const string& expr )
    : FormulaAxis(new THaFormula( name.c_str(), expr.c_str(), false ))
  {
    //TODO: handle compilation error
  }
  InternalFormulaAxis( const InternalFormulaAxis& rhs )
    : FormulaAxis(new THaFormula(*rhs.fFormula)) {}
  InternalFormulaAxis& operator=( const InternalFormulaAxis& rhs )
  {
    if( this != &rhs ) {
      delete fFormula;
      fFormula = new THaFormula(*rhs.fFormula);
    }
    return *this;
  }
#if __cplusplus >= 201103L
  InternalFormulaAxis( InternalFormulaAxis&& rhs ) noexcept
    : FormulaAxis(rhs) { rhs.fFormula = nullptr; }
  InternalFormulaAxis& operator=( InternalFormulaAxis&& rhs ) noexcept
  {
    if( this != & rhs ) {
      delete fFormula;
      fFormula = rhs.fFormula;
      rhs.fFormula = nullptr;
    }
    return *this;
  }
#endif
  virtual ~InternalFormulaAxis() { delete fFormula; }
  virtual InternalFormulaAxis* Clone() const
  {
    return new InternalFormulaAxis(*this);
  }
};

//___________________________________________________________________________
// Axis defined on a single global variable
class VariableAxis : public AxisImpl {
public:
  VariableAxis( const THaVar* pvar, const THaVarList* lst )
    : fVar(pvar), fVarList(lst)
  {
    assert(pvar && lst);
    fName = pvar->GetName();
  }

  virtual VariableAxis* Clone() const
  {
    return new VariableAxis(fVar,fVarList);
  }
  virtual Double_t GetData( Int_t i ) const
  {
    assert( i>=0 && i<GetSize() );
    return fVar->GetValue(i);
  }
  virtual const char* GetName()    const { return fVar->GetName(); }
  virtual Int_t       GetSize()    const { return fVar->GetLen(); }
  virtual Bool_t      IsArray()    const { return fVar->IsArray(); }
  virtual Bool_t      IsVarArray() const { return fVar->IsVarArray(); }
  virtual void        ReAttach()
  {
    fVar = fVarList->Find(fName.c_str());
    //TODO: throw exception if variable not found
    assert( fVar );
  }

protected:
  const THaVar*       fVar;
  const THaVarList*   fVarList;
  string              fName;
};

//=============================================================================
HistogramAxis::HistogramAxis() : fImpl(0)
{
}

//___________________________________________________________________________
HistogramAxis::HistogramAxis( THaFormula* pform )
  : fImpl(new FormulaAxis(pform))
{
}

//___________________________________________________________________________
HistogramAxis::HistogramAxis( const string& name, const string& expr,
			      bool cut )
  : fImpl(0)
{
  HistogramAxis::Init(name,expr,cut);
}

//___________________________________________________________________________
HistogramAxis::HistogramAxis( const THaVar* pvar, const THaVarList* lst )
  : fImpl(new VariableAxis(pvar,lst))
{
}

//_____________________________________________________________________________
HistogramAxis::HistogramAxis( const HistogramAxis& rhs )
{
  // Copy constructor
  if( rhs.fImpl )
    fImpl = rhs.fImpl->Clone();
}

//_____________________________________________________________________________
HistogramAxis& HistogramAxis::operator=( const HistogramAxis& rhs )
{
  // Assignment operator
  if( this != &rhs ) {
    delete fImpl;
    if( rhs.fImpl )
      fImpl = rhs.fImpl->Clone();
    else
      fImpl = 0;
  }
  return *this;
}

#if __cplusplus >= 201103L
//_____________________________________________________________________________
HistogramAxis::HistogramAxis( HistogramAxis&& rhs ) noexcept
  : fImpl(rhs.fImpl)
{
  // Move constructor
  rhs.fImpl = nullptr;
}

//_____________________________________________________________________________
HistogramAxis& HistogramAxis::operator=( HistogramAxis&& rhs ) noexcept
{
  // Move assignment
  if( this != &rhs ) {
    delete fImpl;
    fImpl = rhs.fImpl;
    rhs.fImpl = nullptr;
  }
  return *this;
}
#endif

//___________________________________________________________________________
HistogramAxis::~HistogramAxis()
{
  delete fImpl;
}

//___________________________________________________________________________
Int_t HistogramAxis::Init( THaFormula* pform )
{
  delete fImpl;
  fImpl = new FormulaAxis( pform );
  return 0;
}

//___________________________________________________________________________
Int_t HistogramAxis::Init( const string& name, const string& expr,
			   bool /* cut */ )
{
  delete fImpl;

  Int_t offset;
  if( IsEye(expr,offset) )
    fImpl = new EyeAxis( offset );
  else
    //TODO: use "cut" flag to make a cut axis
    fImpl = new InternalFormulaAxis( name, expr );
  return 0;
}

//___________________________________________________________________________
  Int_t HistogramAxis::Init( const THaVar* pvar, const THaVarList* lst )
{
  delete fImpl;
  fImpl = new VariableAxis( pvar, lst );
  return 0;
}

//___________________________________________________________________________
Double_t HistogramAxis::GetData( Int_t index ) const
{
  return fImpl ? fImpl->GetData(index) : THaVar::kInvalid;
}

//___________________________________________________________________________
const char* HistogramAxis::GetName() const
{
  return fImpl ? fImpl->GetName() : "NOTSET";
}

//___________________________________________________________________________
Int_t HistogramAxis::GetSize() const
{
  return fImpl ? fImpl->GetSize() : 1;
}

//___________________________________________________________________________
Bool_t HistogramAxis::IsScalar() const
{
  return fImpl ? fImpl->IsScalar() : true;
}

//___________________________________________________________________________
Bool_t HistogramAxis::IsArray() const
{
  return fImpl ? fImpl->IsArray() : false;
}

//___________________________________________________________________________
Bool_t HistogramAxis::IsVarArray() const
{
  return fImpl ? fImpl->IsVarArray() : false;
}

//___________________________________________________________________________
Bool_t HistogramAxis::IsEye() const
{
  return fImpl ? (fImpl->GetSize() == -1) : false;
}

//___________________________________________________________________________
void HistogramAxis::ReAttach()
{
  if( fImpl )
    fImpl->ReAttach();
}

} // namespace Output
