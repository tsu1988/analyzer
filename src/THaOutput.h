#ifndef ROOT_THaOutput
#define ROOT_THaOutput

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
//////////////////////////////////////////////////////////////////////////

#define THAOMAX 500
#include "TObject.h"
#include "THaString.h"
#include "TClass.h"
#include "TTree.h"
#include <vector>

class THaFormula;
class THaVar;
class TH1F;
class TH2F;
class THaCut;

class THaOdata {
// Utility class used by THaOutput to store arrays 
// up to size 'nsize' for tree output.
public:
  THaOdata(int n=THAOMAX) : nsize(n) { 
    Class()->IgnoreTObjectStreamer();
    data = new Double_t[n]; Clear();
    memset(data, 0, nsize*sizeof(Double_t)); 
  }
  THaOdata(const THaOdata& other) : nsize(other.nsize) {
    Class()->IgnoreTObjectStreamer();
    data = new Double_t[nsize]; ndata = other.ndata;
    memcpy( data, other.data, nsize*sizeof(Double_t));
  }
  THaOdata& operator=(const THaOdata& rhs) { 
    if( this != &rhs ) {
      if( nsize < rhs.nsize ) {
	nsize = rhs.nsize; delete [] data; data = new Double_t[nsize];
	memset(data, 0, nsize*sizeof(Double_t)); 
      }
      ndata = rhs.ndata; memcpy( data, rhs.data, nsize*sizeof(Double_t));
    }
    return *this;   
  }
  virtual ~THaOdata() { delete [] data; };
  void Clear( Option_t* opt="" ) { ndata = 0; }
  void AddBranches(TTree *T, std::string name) {
     std::string sname = "Ndata." + name;
     std::string leaf = sname;
     T->Branch(sname.c_str(),&ndata,(leaf+"/I").c_str());
     leaf = "data["+leaf+"]/D";
     T->Branch(name.c_str(),data,leaf.c_str());
  }
  Int_t Fill(Int_t i, Double_t dat) {
    if (i >= 0 && i < nsize-1) {
      if (i >= ndata) ndata = i+1;
      data[i] = dat;
      return 1;
    }
    return 0;
  }
  Int_t Fill(Int_t n, const Double_t* array) {
    if( n<0 || n>nsize ) return 0;
    memcpy( data, array, n*sizeof(Double_t));
    ndata = n;
    return 1;
  }
  Int_t       ndata;   // Number of array elements
  Int_t       nsize;   // Maximum number of elements
  Double_t*   data;    // [ndata] Array data

private:

  ClassDef(THaOdata,4)  // Variable sized array
};

class THaOutput {
  
public:

  THaOutput();
  virtual ~THaOutput(); 

  virtual Int_t Init( const char* filename="output.def" );
  virtual Int_t Process();
  virtual Int_t End();
  virtual Bool_t TreeDefined() const { return fTree != 0; };
  virtual TTree* GetTree() const { return fTree; };
#ifdef IFCANWORK
// Preferred method, but doesn't work, hence turned off with ifdef
  virtual Int_t AddToTree(char *name, TObject *tobj); 
#endif

  
  static const Double_t kBig;
  

protected:

  virtual Int_t LoadFile( const char* filename );
  virtual Int_t Attach();
  virtual Int_t FindKey(const THaString& key) const;
  virtual THaString StripBracket(THaString& var) const; 
  virtual void ErrFile(Int_t iden, const THaString& sline) const;
  virtual Int_t ParseTitle(const THaString& sline);
  virtual Int_t BuildBlock(const THaString& blockn);
  void Print() const;

   // Variables, Formulas, Histograms

  std::vector<THaString> fVarnames;           // list contructed from file
  std::vector<THaString> fArrayNames, fVNames; // list of actual working var/arrays
  std::vector<THaString> fFormnames, fFormdef;
  std::vector<THaString> fH1dname, fH1dtit, fH2dname, fH2dtit;
  std::vector<Int_t> fH1dbin, fH2dbinx, fH2dbiny;
  std::vector<Double_t> fH1dxlo, fH1dxhi;
  std::vector<Double_t> fH2dxlo, fH2dxhi, fH2dylo, fH2dyhi;
  Int_t fNform, fNvar, fN1d, fN2d, fNcut;
  std::vector<THaFormula* > fFormulas;
  std::vector<THaVar* > fVariables, fArrays;
  Double_t *fForm, *fVar;
  std::vector<TH1F* > fH1d;
  std::vector<TH2F* > fH2d;
  std::vector<THaString> fH1plot, fH2plotx, fH2ploty;
  std::vector<THaVar* > fH1var, fH2varx, fH2vary;
  std::vector<THaString> fH1cut, fH2cut;
  std::vector<Int_t> fH1cutid, fH2cutid;
  std::vector<THaCut* > fHistCut;
  Int_t *fH1vtype, *fH1form;
  Int_t *fH2vtypex, *fH2formx;
  Int_t *fH2vtypey, *fH2formy;
  std::vector<THaOdata* > fOdata;
  TTree *fTree; 
  bool fInit;

  enum EId { kVar = 1, kForm, kH1, kH2, kBlock };
  static const Int_t kNbout = 4000;
  static const Int_t fgNocut = -1;

private:


   THaOutput(const THaOutput& output) {}
   THaOutput& operator=(const THaOutput& output) { return *this; }

   THaString stitle, sfvar1, sfvar2, scut;
   Int_t n1,n2,iscut;
   float xl1,xl2,xh1,xh2;

   ClassDef(THaOutput,0)  
};

#endif
