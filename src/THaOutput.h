#ifndef ROOT_THaOutput
#define ROOT_THaOutput

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
//////////////////////////////////////////////////////////////////////////

#include "BranchHandlers.h"
#include <vector>
#include <string>

class THaVar;
class TH1F;
class TH2F;
class TBranch;
class THaEvData;
class TTree;
//class THaEvtTypeHandler;
class THaEpicsKey;
class THaEpicsEvtHandler;

namespace Output {
  class Histogram;
  class HistogramAxis;
}

class THaOutput {

public:

  THaOutput();
#if __cplusplus >= 201103L
  THaOutput( const THaOutput& )            = delete;
  THaOutput& operator=( const THaOutput& ) = delete;
  THaOutput( THaOutput&& )                 = delete;
  THaOutput& operator=( THaOutput&& )      = delete;
#endif
  virtual ~THaOutput();

  virtual Int_t  Init( const char* filename="output.def" );
  virtual Int_t  Process();
  virtual Int_t  ProcEpics( THaEvData *ev, THaEpicsEvtHandler *han );
  virtual Int_t  End();
  virtual Bool_t TreeDefined() const { return fTree != 0; };
  virtual TTree* GetTree()     const { return fTree; };

  void SetDebug( Int_t level );

  void Print() const;

protected:

#ifndef __CINT__

  //Moved to BranchHandlers.h
  // enum EId { kInvalidEId = 0, kVar, kForm, kCut, kH1f, kH1d, kH2f, kH2d,
  //	     kBlock, kBegin, kEnd };
  typedef const std::string css_t;
  typedef std::vector<std::string> vs_t;

  struct HistogramParameters {
    HistogramParameters()
      : ikey(Output::kInvalidEId), xlo(0), xhi(0), ylo(0), yhi(0),
	nx(0), ny(0) {}
    Output::EId ikey;
    std::string sname, stitle, sfvarx, sfvary, scut;
    Double_t xlo,xhi,ylo,yhi;
    Int_t nx,ny;
    void clear();
  };

  struct DefinitionSet {
    // Definitions parsed from the configuration file
    vs_t varnames;    // Variable names
    vs_t formnames;   // Formula names
    vs_t formdef;     // Formula definitions
    vs_t cutnames;    // Cut names
    vs_t cutdef;      // Cut definitions
    std::vector<HistogramParameters> histdef; // Histogram definitions
    void clear();
  };

  virtual Int_t       AddBranchName( css_t& sname );
  virtual Int_t       Attach();
  virtual Int_t       BuildBlock( css_t& blockn, DefinitionSet& defs );
  void                BuildList( const vs_t& vdata );
  virtual void        ErrFile( Int_t iden, css_t& sline ) const;
  virtual Output::EId GetKeyID( css_t& key ) const;
  virtual Int_t       MakeAxis( css_t& axis_name, css_t& axis_expr,
				Output::HistogramAxis& axis,
				Bool_t is_cut = false );
  virtual Int_t       LoadFile( const char* filename, DefinitionSet& defs );
  virtual Int_t       ParseHistogramDef( Output::EId key, css_t& sline,
					 HistogramParameters& param );
  virtual Int_t       InitVariables( const DefinitionSet& defs );
  virtual Int_t       InitFormulas(  const DefinitionSet& defs );
  virtual Int_t       InitCuts(      const DefinitionSet& defs );
  virtual Int_t       InitHistos(    const DefinitionSet& defs );
  virtual Int_t       InitEpics(     const DefinitionSet& defs );

  // String handling helper functions
  virtual std::string StripBracket( css_t& var )   const;
  vs_t                reQuote( const vs_t& input ) const;
  std::string         CleanEpicsName( css_t& var ) const;

  // Variables, Formulas, Cuts, Histograms
  Int_t                           fNvar;
  Int_t                           fNform;
  Int_t                           fNcut;
  Output::BranchMap_t             fBranches;
  vs_t                            fBranchNames;
  std::vector<Output::Histogram*> fHistos;
  TTree*                          fTree;
  bool                            fInit;
  Int_t                           fDebug;

  Double_t*                       fEpicsVar;
  std::vector<THaEpicsKey*>       fEpicsKey;
  TTree*                          fEpicsTree;

  static const Int_t kNbout = 4000;

private:
  //  THaEvtTypeHandler *fEpicsHandler;

  Bool_t fOpenEpics,fFirstEpics;

#endif  // ifndef __CINT__

private:
#if __cplusplus < 201103L
  THaOutput( const THaOutput& );
  THaOutput& operator=( const THaOutput& );
#endif

  ClassDef(THaOutput,0)
};

#endif
