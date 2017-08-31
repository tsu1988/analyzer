#ifndef Output_HistogramAxis
#define Output_HistogramAxis

//////////////////////////////////////////////////////////////////////////
//
// Output::HistogramAxis
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <string>

class THaFormula;
class THaVar;
class THaVarList;

namespace Output {

  class AxisImpl;

  //___________________________________________________________________________
  class HistogramAxis {
  public:
    HistogramAxis();
    explicit HistogramAxis( THaFormula* form );
    HistogramAxis( const std::string& name, const std::string& expr,
		   bool cut = false );
    HistogramAxis( const THaVar* var, const THaVarList* lst );
    HistogramAxis( const HistogramAxis& other );
    HistogramAxis& operator=( const HistogramAxis& rhs );
#if __cplusplus >= 201103L
    HistogramAxis( HistogramAxis&& other ) noexcept;
    HistogramAxis& operator=( HistogramAxis&& rhs ) noexcept;
#endif
    ~HistogramAxis();

    Int_t  Init( THaFormula* form );
    Int_t  Init( const std::string& name, const std::string& expr,
		 bool cut = false );
    Int_t  Init( const THaVar* var, const THaVarList* lst );

    Double_t    GetData( Int_t index ) const;
    const char* GetName()      const;
    Int_t       GetSize()      const;
    Bool_t      IsArray()      const;
    Bool_t      IsEye()        const;
    Bool_t      IsInit()       const { return (fImpl != 0); }
    Bool_t      IsFixedArray() const { return (IsArray() && !IsVarArray()); }
    Bool_t      IsScalar()     const;
    Bool_t      IsVarArray()   const;
    void        ReAttach();

    static Bool_t IsEye( const std::string& expr, Int_t& offset );

  private:
    AxisImpl*   fImpl;
  };

} // namespace Output

#endif
