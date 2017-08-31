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
  THaOutput(const THaOutput&) = delete;
  THaOutput& operator=(const THaOutput& ) = delete;
  THaOutput(THaOutput&&) = delete;
  THaOutput& operator=(THaOutput&&) = delete;
#endif
  virtual ~THaOutput();

  virtual Int_t Init( const char* filename="output.def" );
  virtual Int_t Process();
  virtual Int_t ProcEpics(THaEvData *ev, THaEpicsEvtHandler *han);
  virtual Int_t End();
  virtual Bool_t TreeDefined() const { return fTree != 0; };
  virtual TTree* GetTree() const { return fTree; };

  void SetDebug( Int_t level );

  void Print() const;

protected:

#ifndef __CINT__
  //FIXME: to be able to inherit from this class, we need to have the
  // structure/class defintions here, not in the implementation
  struct HistogramParameters;
  struct DefinitionSet;
  // enum EId { kInvalidEId = 0, kVar, kForm, kCut, kH1f, kH1d, kH2f, kH2d,
  //	     kBlock, kBegin, kEnd };

  virtual Int_t LoadFile( const char* filename, DefinitionSet& defs );
  virtual Int_t Attach();
  virtual Output::EId GetKeyID(const std::string& key) const;
  virtual void  ErrFile(Int_t iden, const std::string& sline) const;
  virtual Int_t ParseHistogramDef( Output::EId key, const std::string& sline,
				   HistogramParameters& param );
  virtual Int_t BuildBlock(const std::string& blockn, DefinitionSet& defs);
  virtual std::string StripBracket(const std::string& var) const;
  std::vector<std::string> reQuote(const std::vector<std::string>& input) const;
  std::string CleanEpicsName(const std::string& var) const;
  void BuildList(const std::vector<std::string>& vdata);
  virtual Int_t InitVariables( const DefinitionSet& defs );
  virtual Int_t InitFormulas( const DefinitionSet& defs );
  virtual Int_t InitCuts( const DefinitionSet& defs );
  virtual Int_t InitHistos( const DefinitionSet& defs );
  virtual Int_t InitEpics( const DefinitionSet& defs );
  virtual Int_t AddBranchName( const std::string& sname );
  virtual Int_t MakeAxis( const std::string& axis_name, const std::string& axis_expr,
			  Output::HistogramAxis& axis, Bool_t is_cut = false );

  // Variables, Formulas, Cuts, Histograms
  Int_t fNvar, fNform, fNcut;
  Double_t *fEpicsVar;
  Output::BranchMap_t       fBranches;
  std::vector<std::string>  fBranchNames;
  std::vector<Output::Histogram*>   fHistos;
  std::vector<THaEpicsKey*> fEpicsKey;
  //  std::vector<std::string> fArrayNames;
  //  std::vector<std::string> fVNames;
  TTree *fTree, *fEpicsTree;
  bool  fInit;
  Int_t fDebug;

  static const Int_t kNbout = 4000;

private:
  //  THaEvtTypeHandler *fEpicsHandler;

  Bool_t fOpenEpics,fFirstEpics;

#endif  // ifndef __CINT__

private:
#if __cplusplus < 201103L
  THaOutput(const THaOutput&);
  THaOutput& operator=(const THaOutput& );
#endif

  ClassDef(THaOutput,0)
};

#endif
