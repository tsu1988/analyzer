#ifndef ROOT_THaOutput
#define ROOT_THaOutput

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <vector>
#include <map>
#include <string> 
#include <utility>

class THaVar;
class TH1F;
class TH2F;
class TBranch;
class THaVform;
class THaVhist;
class THaEvData;
class TTree;
//class THaEvtTypeHandler;
class THaEpicsKey;
class THaEpicsEvtHandler;

class THaOdata {
// Utility class used by THaOutput to store arrays 
public:
  THaOdata(int n=1) : tree(NULL), ndata(0), nsize(n)
  { data = new Double_t[n]; }
  THaOdata(const THaOdata& other);
  THaOdata& operator=(const THaOdata& rhs);
  virtual ~THaOdata() { delete [] data; };
  void AddBranches(TTree* T, std::string name);
  void Clear( Option_t* ="" ) { ndata = 0; }  
  Bool_t Resize(Int_t i);
  Int_t Fill(Int_t i, Double_t dat) {
    if( i<0 || (i>=nsize && Resize(i)) ) return 0;
    if( i>=ndata ) ndata = i+1;
    data[i] = dat;
    return 1;
  }
  Int_t Fill(Int_t n, const Double_t* array) {
    if( n<=0 || (n>nsize && Resize(n-1)) ) return 0;
    memcpy( data, array, n*sizeof(Double_t));
    ndata = n;
    return 1;
  }
  Int_t Fill(Double_t dat) { return Fill(0, dat); };
  Double_t Get(Int_t index=0) {
    if( index<0 || index>=ndata ) return 0;
    return data[index];
  }
    
  TTree*      tree;    // Tree that we belong to
  std::string name;    // Name of the tree branch for the data
  Int_t       ndata;   // Number of array elements
  Int_t       nsize;   // Maximum number of elements
  Double_t*   data;    // [ndata] Array data

private:

  //  ClassDef(THaOdata,3)  // Variable sized array
};


class THaOutput {

public:

  THaOutput();
  virtual ~THaOutput(); 

  virtual Int_t Init( const char* filename="output.def" );
  virtual Int_t Process();
  virtual Int_t ProcEpics(THaEvData *ev, THaEpicsEvtHandler *han);
  virtual Int_t End();
  virtual Bool_t TreeDefined() const { return fTree != 0; };
  virtual TTree* GetTree() const { return fTree; };

  static void SetVerbosity( Int_t level );

  void Print() const;
  
protected:

#ifndef __CINT__
  //FIXME: to be able to inherit full from this class, we need to have the
  // structure/class defintions here, not in the implementation
  struct HistogramParameters;
  struct DefinitionSet;
  struct VariableInfo;
  typedef std::map<std::string,VariableInfo> VarMap_t;
  enum EId { kInvalidEId = 0, kVar, kForm, kCut, kH1f, kH1d, kH2f, kH2d,
	     kBlock, kBegin, kEnd };

  virtual Int_t LoadFile( const char* filename, DefinitionSet& defs );
  virtual Int_t Attach();
  virtual EId   GetKeyID(const std::string& key) const;
  virtual void  ErrFile(Int_t iden, const std::string& sline) const;
  virtual Int_t ParseHistogramDef(Int_t key, const std::string& sline,
				  HistogramParameters& param );
  virtual Int_t BuildBlock(const std::string& blockn, DefinitionSet& defs);
  virtual std::string StripBracket(const std::string& var) const; 
  std::vector<std::string> reQuote(const std::vector<std::string>& input) const;
  std::string CleanEpicsName(const std::string& var) const;
  void BuildList(const std::vector<std::string>& vdata);
  // Variables, Formulas, Cuts, Histograms
  //  Int_t fNvar;
  //  Double_t *fVar;
  Double_t *fEpicsVar;
  //  std::vector<THaVar* >  fVariables, fArrays;
  VarMap_t fVariables;
  std::vector<THaVform* > fFormulas, fCuts;
  std::vector<THaVhist* > fHistos;
  std::vector<THaOdata* > fOdata;
  std::vector<THaEpicsKey*>  fEpicsKey;
  std::vector<std::string> fArrayNames;
  std::vector<std::string> fVNames; 
  TTree *fTree, *fEpicsTree; 
  bool fInit;
  
  static const Int_t kNbout = 4000;
  static const Int_t fgNocut = -1;

  static Int_t fgVerbose;

private:
  //  THaEvtTypeHandler *fEpicsHandler;

  Bool_t fOpenEpics,fFirstEpics;

#endif  // ifndef __CINT__

private:
  THaOutput(const THaOutput&);
  THaOutput& operator=(const THaOutput& );

  ClassDef(THaOutput,0)  
};

#endif
