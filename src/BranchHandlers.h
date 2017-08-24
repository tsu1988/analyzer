#ifndef Podd_BranchHandlers
#define Podd_BranchHandlers

//////////////////////////////////////////////////////////////////////////
//
// BranchHandlers. Helper classes for THaOutput.
//
// Podd::VariableHandler
// Podd::FormulaHandler
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <map>
#include <string>

class TTree;
class TBranch;
class THaVar;
class THaVarList;
class THaFormula;

namespace Podd {
  class DataBuffer;
}

namespace Output {

  class BranchHandler;
  class HistogramAxis;
  typedef std::map<std::string,BranchHandler*> BranchMap_t;
  typedef const std::string css_t;

  enum EId { kInvalidEId = 0, kVar, kForm, kCut, kH1f, kH1d, kH2f, kH2d,
	     kBlock, kBegin, kEnd };

  //___________________________________________________________________________
  class BranchHandler {
    // Base class for output branch handlers
  public:
    BranchHandler( EId type = kInvalidEId )
      : fType(type), fBranch(0), fBuffer(0) {}
    BranchHandler( const BranchHandler& other );
    BranchHandler& operator=( const BranchHandler& rhs );
#if __cplusplus >= 201103L
    BranchHandler( BranchHandler&& other );
    BranchHandler& operator=( BranchHandler&& rhs);
#endif
    virtual ~BranchHandler();
    virtual Int_t AddBranch( css_t& branchname,
			     BranchMap_t& branches, TTree* fTree ) = 0;
    virtual void  Attach() {}
    virtual Int_t Fill() = 0;
    EId           GetType()   const { return fType; }
    TBranch*      GetBranch() const { return fBranch; }
    virtual Int_t LinkTo( HistogramAxis& ax ) = 0;
  protected:
    EId      fType;
    TBranch* fBranch;
    Podd::DataBuffer* fBuffer;
  };

//_____________________________________________________________________________
  class VariableHandler : public BranchHandler {
    // Support class for THaVar variables
  public:
    VariableHandler( const THaVar* pvar, const THaVarList* plst,
		     EId type = kVar );
    virtual Int_t AddBranch( css_t& branchname,
			     BranchMap_t& branches, TTree* fTree );
    virtual void  Attach();
    virtual Int_t Fill();
    const THaVar* GetVariable() const { return fVar; }
    virtual Int_t LinkTo( HistogramAxis& ax );
  protected:
    const THaVar*      fVar;
    const THaVarList*  fVarList;
    std::string        fName;
  };

  //_____________________________________________________________________________
  class FormulaHandler : public BranchHandler {
    // Support class for formulas/cuts
  public:
    FormulaHandler( THaFormula* pform, EId type = kForm )
      : BranchHandler(type), fFormula(pform), fNdata(0) {}
    virtual Int_t AddBranch( css_t& branchname,
			     BranchMap_t& branches, TTree* fTree );
    virtual void  Attach();
    virtual Int_t Fill();
    THaFormula*   GetFormula() const { return fFormula; }
    virtual Int_t LinkTo( HistogramAxis& ax );
  protected:
    THaFormula*   fFormula;
    //    union {
      Double_t    fData;     // Result for scalar formulas
    //      Long64_t     fDataI;    // Result for integer-type scalar formulas
    //    };
    Int_t         fNdata;    // Data size for vararray formula
  };

  //___________________________________________________________________________
  class BranchHandlerFactory {
    // Base class for creating output branch handlers (avoids code duplication)
  public:
    BranchHandlerFactory( const char* here, BranchMap_t& branches, TTree* tree )
      : fHere(here), fBranches(branches), fTree(tree) {}
    Int_t AddBranch( css_t& branchname,
		     css_t& definition=std::string() );
    virtual BranchHandler* Create( css_t& branchname, css_t& definition ) = 0;
    virtual ~BranchHandlerFactory() {}
    virtual bool IsEqual( css_t& branchname, css_t& definition,
			  const BranchHandler* existing_handler ) const
    { return false; }
  protected:
    const char*  fHere;
    BranchMap_t& fBranches;
    TTree*       fTree;
  };

  //___________________________________________________________________________
  class VariableHandlerFactory : public BranchHandlerFactory {
    // Factory for variable definitions
  public:
    VariableHandlerFactory( const char* here, BranchMap_t& branches, TTree* tree )
      : BranchHandlerFactory(here,branches,tree) {}
    virtual BranchHandler* Create( css_t& branchname, css_t& definition );
    virtual bool IsEqual(css_t& branchname, css_t& definition,
			 const BranchHandler* existing_handler ) const;
  };

  //___________________________________________________________________________
  class FormulaHandlerFactory : public BranchHandlerFactory {
  public:
    FormulaHandlerFactory( const char* here, BranchMap_t& branches, TTree* tree )
      : BranchHandlerFactory(here,branches,tree) {}
    virtual BranchHandler* Create( css_t& branchname, css_t& definition );
    virtual bool IsEqual(css_t& branchname, css_t& definition,
			 const BranchHandler* existing_handler ) const;
  };

  //___________________________________________________________________________
  class CutHandlerFactory : public FormulaHandlerFactory {
  public:
    CutHandlerFactory( const char* here, BranchMap_t& branches, TTree* tree )
      : FormulaHandlerFactory(here,branches,tree) {}
    virtual BranchHandler* Create( css_t& branchname, css_t& definition );
  };

} // namespace Podd

#endif
