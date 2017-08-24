//*-- AUTHOR :    Robert Michaels   05/08/2002

//////////////////////////////////////////////////////////////////////////
//
// Output::Histogram
//
// Vector of histograms; of course can be just 1 histogram.
// This object uses TH1 class of ROOT to form a histogram
// used by THaOutput.
// The X and Y axis, as well as cut conditions, can be
// formulas of global variables.
// They can also be arrays of variables, or vector formula,
// though certain rules apply about the dimensions; see
// THaOutput documentation for those rules.
//
//
// author:  R. Michaels    May 2003
//
//////////////////////////////////////////////////////////////////////////

#include "Histogram.h"
#include "TH1.h"
#include "TH2.h"
#include "TROOT.h"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <functional>
#include <sstream>

using namespace std;
using namespace Output;

#define ALL(c) (c).begin(), (c).end()

typedef const string& css_t;

//___________________________________________________________________________
struct DeleteObject {
  template< typename T >
  void operator() ( const T* ptr ) const { delete ptr; }
};

//___________________________________________________________________________
template< typename Container >
inline void DeleteContainer( Container& c )
{
  // Delete all elements of given container of pointers
  for_each( ALL(c), DeleteObject() );
  c.clear();
}

//___________________________________________________________________________
struct WriteHist {
  void operator()(TH1* h) { h->Write(); }
};

//___________________________________________________________________________
static Int_t GetCombinedSize( const vector<const HistogramAxis*>& axes,
			      bool require_all_equal )
{
  // Utility function to calculate the number of elements to process
  // for the "histogram axes" given in the 'axes' array.
  // If no axis is an array, the result is 1.
  // Otherwise it is the minimum of the sizes of all the array axes.
  // This can be zero if one or more of the arrays have variable size.
  //
  // If 'require_all_equal' is true, all array axes have to have equal size,
  // otherwise -1 is returned to indicate error.

  Int_t size = kMaxInt;
  const HistogramAxis* prev_array = 0;
  for( vector<const HistogramAxis*>::const_iterator it = axes.begin();
       it != axes.end(); ++it ) {
    const HistogramAxis* ax = *it;
    if( ax->IsArray() ) {
      Int_t theSize = ax->GetSize();
      if( require_all_equal && prev_array != 0 && size != theSize ) {
	cerr << "Error in Output::Histogram: inconsistent sizes of "
	     << prev_array->GetName() << " and " << ax->GetName()
	     << " expressions: "
	     << prev_array->GetSize() << " vs. " << theSize
	     << endl;
	return -1;
      }
      size = min(size,theSize);
      prev_array = ax;
    }
  }
  return (prev_array != 0) ? size : 1;
}

//=============================================================================

namespace Output {

//_____________________________________________________________________________
Histogram::Histogram() : fRequireEqualArrays(false)
{
}

//_____________________________________________________________________________
Histogram::~Histogram()
{
}

//_____________________________________________________________________________
Bool_t Histogram::CheckCut( Int_t index ) const
{
  // Check the cut. Returns true if cut condition passed.

  return (fCut.GetData(index) != 0);
}

//_____________________________________________________________________________
Int_t Histogram::Fill()
{
  // Fill 1D histogram. Must be called exactly once per event

  Int_t ndata = GetSize();
  if( ndata > 0 ) {
    Int_t zero = 0, i = 0;
    Int_t& ic = fCut.IsScalar() ? zero : i;
    for( ; i < ndata; ++i ) {
      if( CheckCut(ic) )
	FillImpl( i, fX.GetData(i) );
    }
  }
  return ndata;
}

//_____________________________________________________________________________
Int_t Histogram::GetSize() const
{
  // Get effective size of x-axis expression and cut, if defined

  assert( fX.IsInit() );

#if __cplusplus >= 201103L
  vector<const HistogramAxis*> axes { &fX };
#else
  const HistogramAxis* r[] = { &fX };
  vector<const HistogramAxis*> axes(r,r+1);
#endif
  if( fCut.IsInit() ) axes.push_back( &fCut );
  return GetCombinedSize(axes,fRequireEqualArrays);
}

//_____________________________________________________________________________
void Histogram::ReAttach()
{
  fX.ReAttach();
  fCut.ReAttach();
}

//_____________________________________________________________________________
void Histogram::RequireEqualArrays( Bool_t b )
{
  fRequireEqualArrays = b;
}

//_____________________________________________________________________________
void Histogram::SetX( const HistogramAxis& xax )
{
  fX = xax;
}

//_____________________________________________________________________________
void Histogram::SetCut( const HistogramAxis& cut )
{
  fCut = cut;
}

#if __cplusplus >= 201103L
//_____________________________________________________________________________
void Histogram::SetX( HistogramAxis&& xax )
{
  fX = move(xax);
}

//_____________________________________________________________________________
void Histogram::SetCut( HistogramAxis&& cut )
{
  fCut = move(cut);
}
#endif

//=============================================================================
Histogram1D::Histogram1D( css_t& name, css_t& title,
			  Int_t nbinx, Double_t xlo, Double_t xhi, bool dbl )
  : fHisto(0)
{
  if( dbl ) {
    fHisto = new TH1D( name.c_str(), title.c_str(), nbinx, xlo, xhi );
  } else {
    fHisto = new TH1F( name.c_str(), title.c_str(), nbinx, xlo, xhi );
  }
}

//_____________________________________________________________________________
Histogram1D::~Histogram1D()
{
  if( TROOT::Initialized() )
    delete fHisto;
}

//_____________________________________________________________________________
void Histogram1D::Clear()
{
  fHisto = 0;
}

//_____________________________________________________________________________
Int_t Histogram1D::End()
{
  assert( fHisto );
  return fHisto->Write();
}

//_____________________________________________________________________________
void Histogram1D::Print() const
{
}

//_____________________________________________________________________________
Int_t Histogram1D::FillImpl( Int_t /* i */, Double_t x )
{
  // Implementation of filling scalar 1D histogram for i-th data point
  // with value x

  assert( fHisto );
  return fHisto->Fill(x);
}

//=============================================================================
MultiHistogram1D::MultiHistogram1D( css_t& name, css_t& title,
    Int_t nhist, Int_t nbinx, Double_t xlo, Double_t xhi, bool dbl )
{
  fHistos.reserve(nhist);
  for( Int_t i = 0; i < nhist; ++i ) {
    ostringstream sname(name); sname << i;
    ostringstream stitle(title); stitle << " " << i;
    if( dbl ) {
      fHistos.push_back( new TH1D(sname.str().c_str(), stitle.str().c_str(),
				  nbinx, xlo, xhi) );
    } else {
      fHistos.push_back( new TH1F(sname.str().c_str(), stitle.str().c_str(),
				  nbinx, xlo, xhi) );
    }
    //TODO: handle errors
  }
  assert( fHistos.size() == static_cast<size_t>(nhist) );
}

//_____________________________________________________________________________
MultiHistogram1D::~MultiHistogram1D()
{
  if( TROOT::Initialized() )
    DeleteContainer(fHistos);
}

//_____________________________________________________________________________
void MultiHistogram1D::Clear()
{
  fHistos.clear();
}

//_____________________________________________________________________________
Int_t MultiHistogram1D::End()
{
  for_each( ALL(fHistos), WriteHist() );
  return 0;
}

//_____________________________________________________________________________
void MultiHistogram1D::Print() const
{
}

//_____________________________________________________________________________
Int_t MultiHistogram1D::FillImpl( Int_t i, Double_t x )
{
  // Implementation of filling multi 1D histograms for i-th data point
  // with value x

  return fHistos.at(i)->Fill(x);
}

//=============================================================================

//_____________________________________________________________________________
Int_t HistogramBase2D::Fill()
{
  // Fill 2D histogram

  Int_t ndata = GetSize();
  if( ndata > 0 ) {
    Int_t zero = 0, i = 0;
    // For scalar variables, retrieve index = 0
    Int_t& ix = fX.IsScalar() ? zero : i;
    Int_t& iy = fY.IsScalar() ? zero : i;
    Int_t& ic = fCut.IsScalar() ? zero : i;
    for( ; i < ndata; ++i ) {
      if( CheckCut(ic) )
	FillImpl2D( i, fX.GetData(ix), fY.GetData(iy) );
    }
  }
  return ndata;
}

//_____________________________________________________________________________
Int_t HistogramBase2D::GetSize() const
{
  // Get effective size of x-axis and y-axis expressions with optional cut

  assert( fX.IsInit() && fY.IsInit() );

#if __cplusplus >= 201103L
  vector<const HistogramAxis*> axes { &fX, &fY };
#else
  const HistogramAxis* r[] = { &fX, &fY };
  vector<const HistogramAxis*> axes(r,r+2);
#endif
  if( fCut.IsInit() ) axes.push_back( &fCut );
  return GetCombinedSize(axes,fRequireEqualArrays);
}

//_____________________________________________________________________________
void HistogramBase2D::ReAttach()
{
  Histogram::ReAttach();
  fY.ReAttach();
}

//_____________________________________________________________________________
void HistogramBase2D::SetY( const HistogramAxis& yax )
{
  fY = yax;
}

#if __cplusplus >= 201103L
//_____________________________________________________________________________
void HistogramBase2D::SetY( HistogramAxis&& yax )
{
  fY = move(yax);
}
#endif

//=============================================================================
Histogram2D::Histogram2D( css_t& name, css_t& title,
			  Int_t nbinx, Double_t xlo, Double_t xhi,
			  Int_t nbiny, Double_t ylo, Double_t yhi, bool dbl )
  : fHisto(0)
{
  if( dbl ) {
    fHisto = new TH2D( name.c_str(), title.c_str(), nbinx, xlo, xhi, nbiny, ylo, yhi );
  } else {
    fHisto = new TH2F( name.c_str(), title.c_str(), nbinx, xlo, xhi, nbiny, ylo, yhi );
  }
}

//_____________________________________________________________________________
Histogram2D::~Histogram2D()
{
  if( TROOT::Initialized() )
    delete fHisto;
}

//_____________________________________________________________________________
void Histogram2D::Clear()
{
  fHisto = 0;
}

//_____________________________________________________________________________
Int_t Histogram2D::End()
{
  assert( fHisto );
  return fHisto->Write();
}

//_____________________________________________________________________________
void Histogram2D::Print() const
{
}

//_____________________________________________________________________________
Int_t Histogram2D::FillImpl2D( Int_t /* i */, Double_t x, Double_t y )
{
  // Implementation of filling a scalar 2D histogram for the i-th data point
  // with values x and y

  assert( fHisto );
  return fHisto->Fill(x,y);
}

//=============================================================================
MultiHistogram2D::MultiHistogram2D( css_t& name, css_t& title, Int_t nhist,
    Int_t nbinx, Double_t xlo, Double_t xhi,
    Int_t nbiny, Double_t ylo, Double_t yhi, bool dbl )
{
  fHistos.reserve(nhist);
  for( Int_t i = 0; i < nhist; ++i ) {
    ostringstream sname(name); sname << i;
    ostringstream stitle(title); stitle << " " << i;
    if( dbl ) {
      fHistos.push_back( new TH2D(sname.str().c_str(), stitle.str().c_str(),
				  nbinx, xlo, xhi, nbiny, ylo, yhi) );
    } else {
      fHistos.push_back( new TH2F(sname.str().c_str(), stitle.str().c_str(),
				  nbinx, xlo, xhi, nbiny, ylo, yhi) );
    }
    //TODO: handle errors
  }
  assert( fHistos.size() == static_cast<size_t>(nhist) );
}

//_____________________________________________________________________________
MultiHistogram2D::~MultiHistogram2D()
{
  if( TROOT::Initialized() )
    DeleteContainer(fHistos);
}

//_____________________________________________________________________________
void MultiHistogram2D::Clear()
{
  fHistos.clear();
}

//_____________________________________________________________________________
Int_t MultiHistogram2D::End()
{
  for_each( ALL(fHistos), WriteHist() );
  return 0;
}

//_____________________________________________________________________________
void MultiHistogram2D::Print() const
{
}

//_____________________________________________________________________________
Int_t MultiHistogram2D::FillImpl2D( Int_t i, Double_t x, Double_t y )
{
  // Implementation of filling multiple 2D histograms for the i-th data point
  // with values x and y

  return fHistos.at(i)->Fill(x,y);
}

} // namespace Output

//_____________________________________________________________________________
//ClassImp(Output::Histogram)
