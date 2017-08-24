#ifndef Output_Histogram
#define Output_Histogram

//////////////////////////////////////////////////////////////////////////
//
// Output::Histogram
//
// Base class for histogram to be written to Podd output file
//
//////////////////////////////////////////////////////////////////////////

#include "HistogramAxis.h"
#include <vector>
#include <string>

class TH1;
class TH2;

namespace Output {

  //enum EHistogramType { kH1F, kH1D, kH2F, kH2D };

  //___________________________________________________________________________

  class Histogram {

  public:

    Bool_t CheckCut( Int_t index = 0 ) const;
    // Clear/unset the histogram objects without deleting,
    // use e.g. when closing ROOT file to prevent double-delete crashes
    virtual void Clear() = 0;
    //    virtual Bool_t IsValid() const { return fValid; }
    // Define axes
    void SetX( const HistogramAxis& xax );
    void SetCut( const HistogramAxis& cut );
#if __cplusplus >= 201103L
    void SetX( HistogramAxis&& xax );
    void SetCut( HistogramAxis&& cut );
#endif
    // Require that multiple arrays (x/y/cut) must all be of equal size
    virtual void RequireEqualArrays( Bool_t b = true );
    // Must ReAttach() if pointers to global variables reset
    virtual void ReAttach();
    // Must Fill() each event.
    virtual Int_t Fill();
    // Must End() to write histogram to output at end of analysis.
    virtual Int_t End() = 0;
    // Self-explanatory printouts.
    virtual void  Print() const {}
    //    virtual void  ErrPrint() const;
    // CheckValid() checks if this histogram is valid.
    // If invalid, you get no output.
    //    virtual Bool_t CheckValidity();
    // IsScaler() is kTRUE if histogram is a vector (which it will then always be)
    virtual Bool_t Is2D()    const { return false; }
    virtual Bool_t IsMulti() const { return false; }
    virtual Int_t  GetSize() const;

#if __cplusplus >= 201103L
    Histogram( const Histogram& ) = delete;
    Histogram& operator=( const Histogram& ) = delete;
    Histogram( Histogram&& ) = delete;
    Histogram& operator=( Histogram&& ) = delete;
#endif
    virtual ~Histogram();

protected:

    Histogram();

    virtual Int_t FillImpl( Int_t i, Double_t x ) = 0;

    HistogramAxis fX;
    HistogramAxis fCut;
    Bool_t fRequireEqualArrays;

private:
#if __cplusplus < 201103L
    Histogram( const Histogram& );
    Histogram& operator=( const Histogram& );
#endif

    //    ClassDef(Output::Histogram,0)

  };

  //___________________________________________________________________________

  class Histogram1D : public Histogram {

  public:
    Histogram1D( const std::string& name, const std::string& title,
		 Int_t nbinx, Double_t xlo, Double_t xhi, bool dbl = true );
    virtual ~Histogram1D();

    virtual void  Clear();
    virtual Int_t End();

    virtual void  Print() const;

  protected:
    virtual Int_t FillImpl( Int_t i, Double_t x );

    TH1* fHisto;
    //    ClassDef(Output::Histogram1D,0)

  };


  //___________________________________________________________________________

  class MultiHistogram1D : public Histogram {

  public:
    MultiHistogram1D( const std::string& name, const std::string& title,
		      Int_t nhist, Int_t nbinx, Double_t xlo, Double_t xhi,
		      bool dbl = true );
    virtual ~MultiHistogram1D();

    virtual void   Clear();
    virtual Int_t  End();

    virtual Bool_t IsMulti() const { return true; }
    virtual void   Print() const;

  protected:
    virtual Int_t  FillImpl( Int_t i, Double_t x );

    std::vector<TH1*> fHistos;
    //    ClassDef(Output::MultiHistogram1D,0)

  };


  //___________________________________________________________________________

  class HistogramBase2D : public Histogram {

  public:
    void SetY( const HistogramAxis& yax );
#if __cplusplus >= 201103L
    void SetY( HistogramAxis&& yax );
#endif
    virtual Int_t  Fill();
    virtual Int_t  GetSize() const;
    virtual Bool_t Is2D() const { return true; }
    virtual void   ReAttach();

  protected:
    virtual Int_t FillImpl2D( Int_t i, Double_t x, Double_t y ) = 0;

    HistogramAxis fY;

  private:
    // Disallow calling this overload for 2D histos
    virtual Int_t FillImpl( Int_t i, Double_t x ) { return Histogram::FillImpl(i,x); }

    //    ClassDef(Output::HistogramBase,0)

  };

  //___________________________________________________________________________

  class Histogram2D : public HistogramBase2D {

  public:
    Histogram2D( const std::string& name, const std::string& title,
		 Int_t nbinx, Double_t xlo, Double_t xhi,
		 Int_t nbiny, Double_t ylo, Double_t yhi, bool dbl = true );
    virtual ~Histogram2D();

    virtual void  Clear();
    virtual Int_t End();
    virtual void  Print() const;

  protected:
    virtual Int_t FillImpl2D( Int_t i, Double_t x, Double_t y );

    TH2* fHisto;
    //    ClassDef(Output::Histogram2D,0)
  };

  //___________________________________________________________________________

  class MultiHistogram2D : public HistogramBase2D {

  public:
    MultiHistogram2D( const std::string& name, const std::string& title,
		      Int_t nhist, Int_t nbinx, Double_t xlo, Double_t xhi,
		      Int_t nbiny, Double_t ylo, Double_t yhi, bool dbl = true );
    virtual ~MultiHistogram2D();

    virtual void   Clear();
    virtual Int_t  End();
    virtual Bool_t IsMulti() const { return true; }
    virtual void   Print() const;

  protected:
    virtual Int_t  FillImpl2D( Int_t i, Double_t x, Double_t y );

    std::vector<TH2*> fHistos;
    //    ClassDef(Output::MultiHistogram2D,0)
  };

} // namespace Output

#endif
