//*-- Author :    Robert Michaels   05/08/2002

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
// Defines the tree and histogram output for THaAnalyzer.
// This class reads a file 'output.def' (an example is in /examples)
// to define which global variables, including arrays, and formulas
// (THaFormula's), and histograms go to the ROOT output.
//
// author:  R. Michaels    Sept 2002
//
//
//////////////////////////////////////////////////////////////////////////

#include "THaOutput.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaFormula.h"
#include "Histogram.h"
#include "THaTextvars.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaEvtTypeHandler.h"
#include "THaEpicsEvtHandler.h"
#include "THaScalerEvtHandler.h"
#include "THaString.h"
#include "THaBenchmark.h"
#include "TROOT.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
#include "TRegexp.h"
#include "TError.h"
#include "TSystem.h"
#include "TString.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>
#include <cstdlib>  // for atoi, atof
#include <memory>

using namespace std;
using namespace Output;
using namespace THaString;

typedef vector<string>::iterator Iter_s_t;
typedef vector<string>::const_iterator Iter_cs_t;
typedef vector<Histogram*>::iterator Iter_h_t;
typedef vector<Histogram*>::const_iterator Iterc_h_t;

//FIXME: these should be member variables
static Bool_t fgDoBench = kTRUE;
static THaBenchmark fgBench;

static const char comment('#');
static const string inctxt("#include");
static const string whitespace(" \t");

#if __cplusplus >= 201103L
# define SMART_PTR unique_ptr
# define STDMOVE(x) std::move(x)
#else
# define SMART_PTR auto_ptr
# define STDMOVE(x) (x)
#endif

#define ALL(c) (c).begin(), (c).end()

//TODO list from 6-Feb-2016:
//
// X Actually process the VariableHandler
// - Allow reassociation of THaVars, e.g. in case we switch to a different
//   (but compatible) variable list from a different thread
// - deal with char* and string/TString variables
// X replace Vform/Vcut stuff with new THaFormulas
// - move ROOT file handling into THaOutput
// - handle event header here
// - add entry numbers of Epics and scaler trees (do we still have those?)
//   to event header
// - support multiple THaOutputs in THaAnalyzer
// - CHECK: can we use formulas/cuts defined here in other formulas/cuts defined here?

//_____________________________________________________________________________
class THaEpicsKey {
// Utility class used by THaOutput to store a list of
// 'keys' to access EPICS data 'string=num' assignments
public:
  THaEpicsKey(const std::string &nm) : fName(nm)
     { fAssign.clear(); }
  void AddAssign(const std::string& input) {
// Add optional assignments.  The input must
// be of the form "epicsvar=number"
// epicsvar can be a string with spaces if
// and only if its enclosed in single spaces.
// E.g. "Yes=1" "No=2", "'RF On'", etc
   typedef map<string, double>::value_type valType;
   string::size_type pos;
   string sdata,temp1,temp2;
   sdata = findQuotes(input);
   pos = sdata.find('=');
   if (pos != string::npos) {
      temp1 = sdata.substr(0,pos);
      temp2 = sdata.substr(pos+1,sdata.length());
// In case someone used "==" instead of "="
      if (temp2.find('=') != string::npos)
	temp2 = sdata.substr(pos+2,sdata.length());
      if (!temp2.empty()) {
	double tdat = atof(temp2.c_str());
	fAssign.insert(valType(temp1,tdat));
      } else {  // In case of "epicsvar="
	fAssign.insert(valType(temp1,0));
     }
   }
  };
  string findQuotes(const string& input) {
    string result,temp1;
    string::size_type pos1,pos2;
    result = input;
    pos1 = input.find('\'');
    if (pos1 != string::npos) {
     temp1 = input.substr(pos1+1,input.length());
     pos2 = temp1.find('\'');
     if (pos2 != string::npos) {
      result = temp1.substr(0,pos2);
      result += temp1.substr(pos2+1,temp1.length());
     }
    }
    return result;
  };
  Bool_t IsString() { return !fAssign.empty(); };
  Double_t Eval(const string& input) {
    if (fAssign.empty()) return 0;
    for (map<string,Double_t>::iterator pm = fAssign.begin();
	 pm != fAssign.end(); ++pm) {
      if (input == pm->first) {
	return pm->second;
      }
    }
    return 0;
  };
  Double_t Eval(const TString& input) {
    return Eval(string(input.Data()));
  }
  const string& GetName() { return fName; };
private:
  string fName;
  map<string,Double_t> fAssign;
};

//___________________________________________________________________________
struct DeleteObject {
  template< typename T >
  void operator() ( T* ptr ) const { delete ptr; }
};

//___________________________________________________________________________
struct ClearObject {
  template< typename T >
  void operator() ( T* ptr ) const { if(ptr) ptr->Clear(); }
};

//___________________________________________________________________________
struct DeleteMapElement {
  template< typename S, typename T >
  void operator() ( pair<S,T*>& elem ) const { delete elem.second; }
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
template< typename Container >
inline void DeleteMap( Container& c )
{
  // Delete all elements of given container of pointers
  for_each( ALL(c), DeleteMapElement() );
  c.clear();
}

//_____________________________________________________________________________
THaOutput::THaOutput() :
  fNvar(0), fNform(0), fNcut(0),
  fEpicsVar(0), fTree(0), fEpicsTree(0), fInit(false), fDebug(3)
{
  // Constructor
}

//_____________________________________________________________________________
THaOutput::~THaOutput()
{
  // Destructor

  // Delete Trees and histograms only if ROOT system is initialized.
  // ROOT will report being uninitialized if we're called from the TSystem
  // destructor, at which point the trees already have been deleted.
  // FIXME: Trees would also be deleted if deleting the output file, right?
  // Can we use this here?
  Bool_t alive = TROOT::Initialized();
  if( alive ) {
    if (fTree) delete fTree;
    if (fEpicsTree) delete fEpicsTree;
    // Deleting the tree should have deleted all the histograms as well,
    // so clear the now-dangling histogram pointers in the Histogram objects
    for_each( ALL(fHistos), ClearObject() );
  }
  if (fEpicsVar) delete [] fEpicsVar;
  DeleteMap(fBranches);
  DeleteContainer(fHistos);
  DeleteContainer(fEpicsKey);
}

//_____________________________________________________________________________
struct THaOutput::HistogramParameters {
  HistogramParameters() : ikey(kInvalidEId), xlo(0), xhi(0), ylo(0), yhi(0),
			  nx(0), ny(0) {}
  EId ikey;
  string sname, stitle, sfvarx, sfvary, scut;
  Double_t xlo,xhi,ylo,yhi;
  Int_t nx,ny;
  void clear() {
    ikey = kInvalidEId;
    sname.clear(); stitle.clear(); sfvarx.clear(); sfvary.clear(); scut.clear();
    nx = ny = xlo = xhi = ylo = yhi = 0;
  }
};

//_____________________________________________________________________________
struct THaOutput::DefinitionSet {
  // Definitions parsed from the configuration file
  vector<string> varnames;    // Variable names
  vector<string> formnames;   // Formula names
  vector<string> formdef;     // Formula definitions
  vector<string> cutnames;    // Cut names
  vector<string> cutdef;      // Cut definitions
  vector<HistogramParameters> histdef; // Histogram definitions
  void clear() {
    varnames.clear(); formnames.clear(); formdef.clear();
    cutnames.clear(); cutdef.clear(); histdef.clear();
  }
};

//_____________________________________________________________________________
Int_t THaOutput::InitVariables( const DefinitionSet& defs )
{
  // Initialize variable branches
  const char* const here = "THaOutput::InitVariables";

  BranchHandlerFactory* fac = new VariableHandlerFactory(here,fBranches,fTree);
  for( Iter_cs_t it = defs.varnames.begin(); it != defs.varnames.end(); ++it ) {
    const string& branchname = *it;
    Int_t ret = fac->AddBranch(branchname);
    if( ret == 0 )
      ++fNvar;
  }
  delete fac;
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::InitFormulas( const DefinitionSet& defs )
{
  // Initialize formula branches
  const char* const here = "THaOutput::InitFormulas";

  Int_t k = 0;
  BranchHandlerFactory* fac = new FormulaHandlerFactory(here,fBranches,fTree);
  for( Iter_cs_t it = defs.formnames.begin(); it != defs.formnames.end(); ++it, ++k ) {
    const string& branchname = *it;
    const string& formdef = defs.formdef[k];
    Int_t ret = fac->AddBranch(branchname,formdef);
    if( ret == 0 )
      ++fNform;
  }
  delete fac;
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::InitCuts( const DefinitionSet& defs )
{
  // Cuts initialization. Cuts are just special formulas
  const char* const here = "THaOutput::InitCuts";

  Int_t k = 0;
  BranchHandlerFactory* fac = new CutHandlerFactory(here,fBranches,fTree);
  for( Iter_cs_t it = defs.cutnames.begin(); it != defs.cutnames.end(); ++it, ++k ) {
    const string& branchname = *it;
    const string& cutdef = defs.cutdef[k];
    Int_t ret = fac->AddBranch(branchname,cutdef);
    if( ret == 0 )
      ++fNcut;
  }
  delete fac;
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::MakeAxis( const string& axis_name, const string& axis_expr,
			   HistogramAxis& axis )
{
  // Initialize the histogram axis/cut handler 'axis' from the
  // expression'axis_expr'

  //TODO: original code behavior:
  // - case insensitive formula/cut name comparison! (make case-insensitivity configurable?)
  // - only search formulas for x/y-axes, only search cuts for cut "axis"

  if( axis_expr.empty() )
    return 0;
  Int_t ret;
  BranchMap_t::iterator it = fBranches.find( axis_expr );
  if( it != fBranches.end() ) {
    // If axis_expr matches a defined variable, cut or formula exactly, use it
    ret = it->second->LinkTo( axis );
  } else {
    // Otherwise, create a new formula (or possibly an "eye")
    ret = axis.Init( axis_name, axis_expr );
  }
  return ret;
}

//_____________________________________________________________________________
Int_t THaOutput::InitHistos( const DefinitionSet& defs )
{
  // Initialize histograms
  const char* const here = "THaOutput::InitHistos";

  for( vector<HistogramParameters>::const_iterator it = defs.histdef.begin();
       it != defs.histdef.end(); ++it ) {

    const HistogramParameters& hpar = *it;
    if( hpar.sfvarx.empty() )
      continue;

    Histogram* h;
    HistogramAxis xax, yax, cut;
    MakeAxis( hpar.sname+"X",   hpar.sfvarx, xax );
    MakeAxis( hpar.sname+"Cut", hpar.scut,   cut );
    //TODO: check for errors, skip on error

    EId key = hpar.ikey;
    bool dbl = ( key == kH1D || key == kH2D || key == kvH1D || key == kvH2D );
    bool scalar = ( key == kH1F || key == kH1D || key == kH2F || key == kH2D );
    Int_t nhist = 0;
    switch( key ) {
      // 1D histograms
      // Creation rules:
      //  S = scalar, A = fixed array, V = variable array, I = [I] (Iteration$)
      // The cut cannot be I. It makes no sense.
      // "new": not supported in original code
      //
      //  xax  cut  -> histo   notes
      //   S    -        S
      //   S    S        S
      //   S    A        S     different, all cut[i] (Bob: size=1 vector histogram?)
      //   S    V        S     different, all cut[i] (Bob: cut[0] (may not exist))
      //   A    -        S     all x[i]  (A for consistency with 2D?)
      //   A    S        S     all x[i]  (A for consistency with 2D?)
      //   A    A        A     require sizex == sizec
      //   A    V        S     new, normal ROOT behavior: index runs up to min(sizex,sizec)
      //   V    -        S
      //   V    S        S
      //   V    A        S    /new, normal ROOT behavior: index runs up to min(sizex,sizec)
      //   V    V        S    \(Bob: ill-defined, sizex==sizec and sizex!=sizec different)
      //   I    -    disallowed
      //   I    S        S     new, distribution of cut[0] (for consistency)
      //   I    A        S     new, distribution of cut(index)
      //   I    V        S     new, distribution of cut(index)
    case kvH1D:
    case kvH1F:
      // For vector histograms, the x-axis and cut must have the same size
      if( xax.IsFixedArray() && cut.IsFixedArray() ) {
	if( xax.GetSize() != cut.GetSize() ) {
	  Error( here, "Histogram %s: Inconsistent cut size", hpar.sname.c_str() );
	  continue;
	}
	nhist = xax.GetSize();
	if( nhist > 1 ) {
	  h = new MultiHistogram1D( hpar.sname, hpar.stitle, nhist,
				    hpar.nx, hpar.xlo, hpar.xhi, dbl );
#ifndef NDEBUG
	  h->RequireEqualArrays(true);
#endif
	  break;
	}
      }
      // If both axes are not fixed arrays or nhist == 1, fall through
      // to scalar histogram case
    case kH1F:
    case kH1D:
      if( cut.IsEye() || (xax.IsEye() && !cut.IsInit()) ) {
	Error( here, "Histogram %s: Illegal [I] expression", hpar.sname.c_str() );
	continue;
      }

      h = new Histogram1D( hpar.sname, hpar.stitle,
			   hpar.nx, hpar.xlo, hpar.xhi, dbl );
      break;

      // 2D histograms
      // Creation rules: (assuming x/y symmetry)
      //
      //  xax  yax cut  -> histo   notes
      //   S    S   -        S
      //   S    S   S        S
      //   S    S   A        S     all cut[i] (Bob: size=1 vector histogram)
      //   S    S   V        S     all cut[i] (Bob: undefined if sizec==0?)
      //   S    A   -        S     all y[i]   (Bob: only y[0] BUG?)
      //   S    A   S        S     all y[i]   (Bob: only y[0] BUG?)
      //   S    A   A        A     require sizey == sizec
      //   S    A   V        S     y[i], cut[i] parallel
      //   S    V   -        S     all y[i]
      //   S    V   S        S     "
      //   S    V   A        S     y[i], cut[i] parallel
      //   S    V   V        S     "
      //   A    A   -        A     require sizex == sizey
      //   A    A   S        A     "
      //   A    A   A        A     require sizex == sizey == sizec
      //   A    A   V        S     vector histogram would be ill-defined
      //   A    V   -        S
      //   A    V   S        S
      //   A    V   A        S     vector histogram would be ill-defined
      //   A    V   V        S
      //   V    V   -        S
      //   V    V   S        S
      //   V    V   A        S     (Bob: ill-defined vector?)
      //   V    V   V        S
      //   I    S   -        S
      //   I    S   S        S
      //   I    S   A        S
      //   I    S   V        S
      //   I    A   -        S
      //   I    A   S        S
      //   I    A   A        A    require sizey == sizec
      //   I    A   V        S    vector histogram would be ill-defined
      //   I    V   -        S
      //   I    V   S        S
      //   I    V   A        S    (Bob: ill-defined vector?)
      //   I    V   V        S
      //   I    I   -   disallowed
      //   I    I   S        S
      //   I    I   A        S    (Bob: ill-defined vector?)
      //   I    I   V        S

    case kH2F:
    case kH2D:
    case kvH2D:
    case kvH2F:
      MakeAxis( hpar.sname+"Y",   hpar.sfvary, yax );
      // TODO: handle MakeAxis errors

      // Test for disallowed combinations
      if( cut.IsEye() || (xax.IsEye() && yax.IsEye() && !cut.IsInit()) ) {
	Error( here, "Histogram %s: Illegal [I] expression(s)",
	       hpar.sname.c_str() );
	continue;
      }
      // Create a multi-histogram if two or more axes are fixed arrays,
      // unless the user has explicitly asked for a single (scalar) one
      // or one of the axes has variable size.
      // All fixed arrays must have the same size.
      if( !scalar ) {
	if( xax.IsVarArray() || yax.IsVarArray() || cut.IsVarArray() ) {
	  scalar = true;
	} else {
	  HistogramAxis* axes[] = { &xax, &yax, &cut };
	  HistogramAxis* prev_fixed = 0;
	  Int_t nfixed = 0;
	  for( Int_t i = 0; i < 3; ++i ) {
	    HistogramAxis* theAxis = axes[i];
	    if( theAxis->IsFixedArray() ) {
	      if( prev_fixed && prev_fixed->GetSize() != theAxis->GetSize() ) {
		Error( here, "Histogram %s: Inconsistent expression sizes "
		       "%s (%d) vs. %s (%d)", hpar.sname.c_str(),
		       prev_fixed->GetName(), prev_fixed->GetSize(),
		       theAxis->GetName(), theAxis->GetSize() );
		continue;
	      }
	      nhist = theAxis->GetSize();
	      prev_fixed = theAxis;
	      ++nfixed;
	    }
	  }
	  if( nfixed < 2 )
	    scalar = true;
	}
      }

      if( scalar || nhist == 1 ) {
	h = new Histogram2D( hpar.sname, hpar.stitle,
			     hpar.nx, hpar.xlo, hpar.xhi,
			     hpar.ny, hpar.ylo, hpar.yhi, dbl );
      } else {
	assert( nhist > 0 );
	h = new MultiHistogram2D( hpar.sname, hpar.stitle, nhist,
				  hpar.nx, hpar.xlo, hpar.xhi,
				  hpar.ny, hpar.ylo, hpar.yhi, dbl );
#ifndef NDEBUG
	h->RequireEqualArrays(true);
#endif
      }

      assert( dynamic_cast<HistogramBase2D*>(h) );
      static_cast<HistogramBase2D*>(h)->SetY( STDMOVE(yax) );

      break;
    default:
      assert(false);  // otherwise hpar not filled correctly
      break;
    }

    h->SetX( STDMOVE(xax) );
    h->SetCut( STDMOVE(cut) );

    fHistos.push_back(h);
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::InitEpics( const DefinitionSet& /* defs */ )
{
  // Add branches for EPICS keys
  //TODO: don't add to event tree since we already have an EPICS tree?
  if (!fEpicsKey.empty()) {
    vector<THaEpicsKey*>::size_type siz = fEpicsKey.size();
    fEpicsVar = new Double_t[siz+1];
    UInt_t i = 0;
    for (vector<THaEpicsKey*>::iterator it = fEpicsKey.begin();
	 it != fEpicsKey.end(); ++it, ++i) {
      fEpicsVar[i] = -1e32;
      string epicsbr = CleanEpicsName((*it)->GetName());
      string tinfo = epicsbr + "/D";
      fTree->Branch(epicsbr.c_str(), &fEpicsVar[i],
	tinfo.c_str(), kNbout);
      fEpicsTree->Branch(epicsbr.c_str(), &fEpicsVar[i],
	tinfo.c_str(), kNbout);
    }
    fEpicsVar[siz] = -1e32;
    fEpicsTree->Branch("timestamp",&fEpicsVar[siz],"timestamp/D", kNbout);
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::Init( const char* filename )
{
  // Initialize output system. Required before anything can happen.

  // const char* const here = "THaOutput::Init";

  // Only re-attach to variables and re-compile cuts and formulas
  // upon re-initialization (eg: continuing analysis with another file)
  if( fInit ) {
    cout << "\nTHaOutput::Init: Info: THaOutput cannot be completely"
	 << " re-initialized. Keeping existing definitions." << endl;
    cout << "Global Variables are being re-attached and formula/cuts"
	 << " are being re-compiled." << endl;

    // Assign pointers and recompile stuff reliant on pointers.

    if ( Attach() )
      return -4;

    Print();
    return 1;
  }

  if( !gHaVars ) return -2;

  if( fgDoBench ) fgBench.Begin("Init");

  fOpenEpics  = kFALSE;
  fFirstEpics = kTRUE;

  DefinitionSet defs;
  Int_t err = LoadFile( filename, defs );
  if( err != -1 && err != 0 ) {
    if( fgDoBench ) fgBench.Stop("Init");
    return -3;
  }

  fTree = new TTree("T","Hall A Analyzer Output DST");
  fTree->SetAutoSave(200000000);

  if( err == -1 ) {
    // No error if file not found, but read the instructions.
    if( fgDoBench ) fgBench.Stop("Init");
    return 0;
  }

  InitVariables( defs );
  InitFormulas( defs);
  InitCuts( defs );
  InitHistos( defs );
  InitEpics( defs );

  Print();

  fInit = true;

  if( fgDoBench ) fgBench.Stop("Init");

  if( fgDoBench ) fgBench.Begin("Attach");
  Int_t st = Attach();
  if( fgDoBench ) fgBench.Stop("Attach");
  if ( st )
    return -4;

  return 0;
}

//_____________________________________________________________________________
void THaOutput::BuildList( const vector<string>& vdata)
{
  // Build list of EPICS variables and
  // SCALER variables to add to output.

    if (vdata.empty()) return;
    if (CmpNoCase(vdata[0],"begin") == 0) {
      if (vdata.size() < 2) return;
      if (CmpNoCase(vdata[1],"epics") == 0) fOpenEpics = kTRUE;
    }
    if (CmpNoCase(vdata[0],"end") == 0) {
      if (vdata.size() < 2) return;
      if (CmpNoCase(vdata[1],"epics") == 0) fOpenEpics = kFALSE;
    }
    if (fOpenEpics) {
       cout << "THaOutput::ERROR: Syntax error in output.def"<<endl;
       cout << "Must 'begin' and 'end' before 'begin' again."<<endl;
       cout << "e.g. 'begin epics' ..... 'end epics'"<<endl;
       return ;
    }
    if (fOpenEpics) {
       if (fFirstEpics) {
	   if (!fEpicsTree)
	      fEpicsTree = new TTree("E","Hall A Epics Data");
	   fFirstEpics = kFALSE;
	   return;
       } else {
	  fEpicsKey.push_back(new THaEpicsKey(vdata[0]));
	  if (vdata.size() > 1) {
	    std::vector<string> esdata = reQuote(vdata);
	    for (int k = 1; k < (int)esdata.size(); k++) {
	      fEpicsKey[fEpicsKey.size()-1]->AddAssign(esdata[k]);
	    }
	  }
       }
    } else {
       fFirstEpics = kTRUE;
    }
    return;
}


//_____________________________________________________________________________
Int_t THaOutput::Attach()
{
  // Get the pointers for the global variables
  // Also, sets the size of the fVariables and fArrays vectors
  // according to the size of the related names array

  if( !gHaVars ) return -2;

  //FIXME: TODO

  // THaVar *pvar;
  // Int_t NAry = fArrayNames.size();
  // Int_t NVar = fVNames.size();

  // fVariables.resize(NVar);
  // fArrays.resize(NAry);

  // // simple variable-type names
  // for (Int_t ivar = 0; ivar < NVar; ivar++) {
  //   pvar = gHaVars->Find(fVNames[ivar].c_str());
  //   if (pvar) {
  //     if ( !pvar->IsArray() ) {
  //	fVariables[ivar] = pvar;
  //     } else {
  //	cout << "\tTHaOutput::Attach: ERROR: Global variable " << fVNames[ivar]
  //	     << " changed from simple to array!! Leaving empty space for variable"
  //	     << endl;
  //	fVariables[ivar] = 0;
  //     }
  //   } else {
  //     cout << "\nTHaOutput::Attach: WARNING: Global variable ";
  //     cout << fVarnames[ivar] << " NO LONGER exists (it did before). "<< endl;
  //     cout << "This is not supposed to happen... "<<endl;
  //   }
  // }

  // // arrays
  // for (Int_t ivar = 0; ivar < NAry; ivar++) {
  //   pvar = gHaVars->Find(fArrayNames[ivar].c_str());
  //   if (pvar) {
  //     if ( pvar->IsArray() ) {
  //	fArrays[ivar] = pvar;
  //     } else {
  //	cout << "\tTHaOutput::Attach: ERROR: Global variable " << fVNames[ivar]
  //	     << " changed from ARRAY to Simple!! Leaving empty space for variable"
  //	     << endl;
  //	fArrays[ivar] = 0;
  //     }
  //   } else {
  //     cout << "\nTHaOutput::Attach: WARNING: Global variable ";
  //     cout << fVarnames[ivar] << " NO LONGER exists (it did before). "<< endl;
  //     cout << "This is not supposed to happen... "<<endl;
  //   }
  // }

  // Reattach formulas, cuts, histos

#if 0
  for (Iter_f_t iform=fFormulas.begin(); iform!=fFormulas.end(); ++iform) {
    (*iform)->ReAttach();
  }

  for (Iter_f_t icut=fCuts.begin(); icut!=fCuts.end(); ++icut) {
    (*icut)->ReAttach();
  }
#endif

  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist) {
    (*ihist)->ReAttach();
  }

  return 0;

}

//_____________________________________________________________________________
Int_t THaOutput::ProcEpics(THaEvData *evdata, THaEpicsEvtHandler *epicshandle)
{
  // Process the EPICS data, this fills the trees.

  if ( !epicshandle ) return 0;
  if ( !epicshandle->IsMyEvent(evdata->GetEvType())
       || fEpicsKey.empty() || !fEpicsTree ) return 0;
  if( fgDoBench ) fgBench.Begin("EPICS");
  fEpicsVar[fEpicsKey.size()] = -1e32;
  for (UInt_t i = 0; i < fEpicsKey.size(); i++) {
    if (epicshandle->IsLoaded(fEpicsKey[i]->GetName().c_str())) {
      //      cout << "EPICS name "<<fEpicsKey[i]->GetName()<<"    val "<< epicshandle->GetString(fEpicsKey[i]->GetName().c_str())<<endl;
      if (fEpicsKey[i]->IsString()) {
	fEpicsVar[i] = fEpicsKey[i]->Eval(
	  epicshandle->GetString(
	     fEpicsKey[i]->GetName().c_str()));
      } else {
	fEpicsVar[i] =
	   epicshandle->GetData(
	      fEpicsKey[i]->GetName().c_str());
      }
 // fill time stamp (once is ok since this is an EPICS event)
      fEpicsVar[fEpicsKey.size()] =
	 epicshandle->GetTime(
	   fEpicsKey[i]->GetName().c_str());
    } else {
      fEpicsVar[i] = -1e32;  // data not yet found
    }
  }
  if (fEpicsTree != 0) fEpicsTree->Fill();
  if( fgDoBench ) fgBench.Stop("EPICS");
  return 1;
}

//_____________________________________________________________________________
Int_t THaOutput::Process()
{
  // Process the variables, formulas, and histograms.
  // This is called by THaAnalyzer.

  if( fgDoBench ) fgBench.Begin("Branches");
  // Process the variable/formula/cut branches exactly in the order in
  // which they were defined in the definition file(s)
  for( vector<string>::iterator it = fBranchNames.begin();
       it != fBranchNames.end(); ++it ) {
    BranchMap_t::iterator bt = fBranches.find(*it);
    // A branch name may not exist in fBranches because it couldn't be initialized
    if( bt != fBranches.end() )
      bt->second->Fill();
  }
  if( fgDoBench ) fgBench.Stop("Branches");

  //  Int_t k = 0;
  // for (Iter_o_t it = fOdata.begin(); it != fOdata.end(); ++it, ++k) {
  //   THaVar* pvar = fArrays[k];
  //   if ( pvar == NULL ) continue;
  //   THaOdata* pdat(*it);
  //   pdat->Clear();
  //   // Fill array in reverse order so that fOdata[k] gets resized just once
  //   Int_t i = pvar->GetLen();
  //   bool first = true;
  //   while( i-- > 0 ) {
  //     // FIXME: for better efficiency, should use pointer to data and
  //     // Fill(int n,double* data) method in case of a contiguous array
  //     if (pdat->Fill(i,pvar->GetValue(i)) != 1) {
  //	if( fDebug>0 && first ) {
  //	  cerr << "THaOutput::ERROR: storing too much variable sized data: "
  //	       << pvar->GetName() <<"  "<<pvar->GetLen()<<endl;
  //	  first = false;
  //	}
  //     }
  //   }
  // }

  if( fgDoBench ) fgBench.Begin("Histos");
  for ( Iter_h_t it = fHistos.begin(); it != fHistos.end(); ++it )
    (*it)->Fill();
  if( fgDoBench ) fgBench.Stop("Histos");

  if( fgDoBench ) fgBench.Begin("TreeFill");
  if (fTree != 0) fTree->Fill();
  if( fgDoBench ) fgBench.Stop("TreeFill");

  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::End()
{
  if( fgDoBench ) fgBench.Begin("End");

  if (fTree != 0) fTree->Write();
  if (fEpicsTree != 0) fEpicsTree->Write();
  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist)
    (*ihist)->End();
  if( fgDoBench ) fgBench.Stop("End");

  if( fgDoBench ) {
    cout << "Output timing summary:" << endl;
    fgBench.Print("Init");
    fgBench.Print("Attach");
    fgBench.Print("Branches");
    fgBench.Print("Histos");
    fgBench.Print("TreeFill");
    fgBench.Print("EPICS");
    fgBench.Print("End");
  }
  return 0;
}

//_____________________________________________________________________________
inline static Int_t GetIncludeFileName( const string& line, string& incfile )
{
  // Extract file name from #include statement

  if( line.substr(0,inctxt.length()) != inctxt ||
      line.length() <= inctxt.length() )
    return -1; // Not an #include statement (should never happen)
  string::size_type pos = line.find_first_not_of(whitespace,inctxt.length()+1);
  if( pos == string::npos || (line[pos] != '<' && line[pos] != '\"') )
    return -2; // No proper file name delimiter
  const char end = (line[pos] == '<') ? '>' : '\"';
  string::size_type pos2 = line.find(end,pos+1);
  if( pos2 == string::npos || pos2 == pos+1 )
    return -3; // Unbalanced delimiters or zero length filename
  if( line.length() > pos2 &&
      line.find_first_not_of(whitespace,pos2+1) != string::npos )
    return -4; // Trailing garbage after filename spec

  incfile = line.substr(pos+1,pos2-pos-1);
  return 0;
}

//_____________________________________________________________________________
inline static Int_t CheckIncludeFilePath( string& incfile )
{
  // Check if 'incfile' can be opened in the current directory or
  // any of the directories in the include path

  vector<TString> paths;
  paths.push_back( incfile.c_str() );

  TString incp = gSystem->Getenv("ANALYZER_CONFIGPATH");
  if( !incp.IsNull() ) {
    SMART_PTR<TObjArray> incdirs( incp.Tokenize(":") );
    if( !incdirs->IsEmpty() ) {
      Int_t ndirs = incdirs->GetLast()+1;
      assert( ndirs>0 );
      for( Int_t i = 0; i < ndirs; ++i ) {
	TString path = (static_cast<TObjString*>(incdirs->At(i)))->String();
	if( path.IsNull() )
	  continue;
	if( !path.EndsWith("/") )
	  path.Append("/");
	path.Append( incfile.c_str() );
	paths.push_back( path.Data() );
      }
    }
  }

  for( vector<TString>::size_type i = 0; i<paths.size(); ++i ) {
    TString& path = paths[i];
    if( !gSystem->ExpandPathName(path) &&
	!gSystem->AccessPathName(path,kReadPermission) ) {
      incfile = path.Data();
      return 0;
    }
  }
  return -1;
}

//_____________________________________________________________________________
Int_t THaOutput::AddBranchName( const string& sname )
{
  // Add name of branch definition (variable, formula, cut) to the list of
  // branches that preserves the order in which things are defined. If a name
  // already exists, it is not added again.

  if( find(ALL(fBranchNames), sname) != fBranchNames.end() )
    return 1;
  fBranchNames.push_back(sname);
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::LoadFile( const char* filename, DefinitionSet& defs )
{
  // Process the file that defines the output

  const char* const here = "THaOutput::LoadFile";

  if( !filename || !*filename || strspn(filename," ") == strlen(filename) ) {
    ::Error( here, "invalid file name, no output definition loaded" );
    return -2;
  }
  string loadfile(filename);
  ifstream odef(loadfile.c_str());
  if ( !odef ) {
    ErrFile(-1, loadfile);
    return -1;
  }
  string::size_type pos;
  vector<string> strvect;
  string sline;
  while (getline(odef,sline)) {
    // #include
    if( sline.substr(0,inctxt.length()) == inctxt &&
	sline.length() > inctxt.length() ) {
      string incfilename;
      if( GetIncludeFileName(sline,incfilename) != 0 ) {
	ostringstream ostr;
	ostr << "Error in #include specification: " << sline;
	::Error( here, "%s", ostr.str().c_str() );
	return -3;
      }
      if( CheckIncludeFilePath(incfilename) != 0 ) {
	ostringstream ostr;
	ostr << "Error opening include file: " << sline;
	::Error( here, "%s", ostr.str().c_str() );
	return -3;
      }
      if( incfilename == filename ) {
	// File including itself?
	// FIXME: does not catch including the same file via full pathname or similar
	ostringstream ostr;
	ostr << "File cannot include itself: " << sline;
	::Error( here, "%s", ostr.str().c_str() );
	return -3;
      }
      Int_t ret = LoadFile( incfilename.c_str(), defs );
      if( ret != 0 )
	return ret;
      continue;
    }
    // Blank line or comment line?
    if( sline.empty() ||
	(pos = sline.find_first_not_of(whitespace)) == string::npos ||
	sline[pos] == comment )
      continue;
    // Get rid of trailing comments
    if( (pos = sline.find(comment)) != string::npos )
      sline.erase(pos);
    // Substitute text variables
    vector<string> lines( 1, sline );
    if( gHaTextvars->Substitute(lines) )
      continue;
    for( Iter_s_t it = lines.begin(); it != lines.end(); ++it ) {
      // Split the line into tokens separated by whitespace
      const string& str = *it;
      strvect = Split(str);
      //TODO: check this...
      bool special_before = fOpenEpics;
      BuildList(strvect);
      bool special_now = fOpenEpics;
      if( special_before || special_now )
	continue; // strvect already processed
      if (strvect.size() < 2) {
	ErrFile(0, str);
	continue;
      }
      EId ikey = GetKeyID( strvect[0] );
      string sname = StripBracket(strvect[1]);
      switch (ikey) {
      case kVar:
	AddBranchName(sname);
	defs.varnames.push_back(STDMOVE(sname));
	break;
      case kForm:
	if (strvect.size() < 3) {
	  ErrFile(ikey, str);
	  continue;
	}
	AddBranchName(sname);
	defs.formnames.push_back(STDMOVE(sname));
	defs.formdef.push_back(STDMOVE(strvect[2]));
	break;
      case kCut:
	if (strvect.size() < 3) {
	  ErrFile(ikey, str);
	  continue;
	}
	AddBranchName(sname);
	defs.cutnames.push_back(STDMOVE(sname));
	defs.cutdef.push_back(STDMOVE(strvect[2]));
	break;
      case kH1F:
      case kH1D:
      case kH2F:
      case kH2D:
      case kvH1F:
      case kvH1D:
      case kvH2F:
      case kvH2D:
	{
	  HistogramParameters hpar;
	  if( ParseHistogramDef(ikey, str, hpar) != 1) {
	    ErrFile(ikey, str);
	    continue;
	  }
	  defs.histdef.push_back(STDMOVE(hpar));
	}
	break;
      case kBlock:
	// Do not strip brackets for block regexps: use strvect[1] not sname
	if( BuildBlock(strvect[1],defs) == 0 ) {
	  ostringstream ostr;
	  ostr << "Block " << strvect[1] << " does not match any variables."
	       << endl << "There is probably a typo error... ";
	  ::Warning( here, "%s", ostr.str().c_str() );
	}
	break;
      case kBegin:
      case kEnd:
	break;
      case kInvalidEId:
	{
	  ostringstream ostr;
	  ostr << "Keyword " << strvect[0] << " undefined. Line ignored.";
	  ::Warning( here, "%s", ostr.str().c_str() );
	}
	break;
      }
    }
  }
  return 0;
}

//_____________________________________________________________________________
Output::EId THaOutput::GetKeyID(const string& key) const
{
  // Return integer flag corresponding to
  // case-insensitive keyword "key" if it exists

  // Map of keywords to internal logical type numbers
  struct KeyMap {
    const char* name;
    EId keyval;
  };
  static const KeyMap keymap[] = {
    { "variable", kVar },
    { "formula",  kForm },
    { "cut",      kCut },
    { "th1f",     kvH1F },
    { "th1d",     kvH1D },
    { "th2f",     kvH2F },
    { "th2d",     kvH2D },
    { "sth1f",    kH1F },
    { "sth1d",    kH1D },
    { "sth2f",    kH2F },
    { "sth2d",    kH2D },
    { "vth1f",    kvH1F },
    { "vth1d",    kvH1D },
    { "vth2f",    kvH2F },
    { "vth2d",    kvH2D },
    { "block",    kBlock },
    { "begin",    kBegin },
    { "end",      kEnd },
    { 0 }
  };

  if( const KeyMap* it = keymap ) {
    while( it->name ) {
      if( CmpNoCase( key, it->name ) == 0 )
	return it->keyval;
      ++it;
    }
  }
  return kInvalidEId;
}

//_____________________________________________________________________________
string THaOutput::StripBracket(const string& var) const
{
  // If the string contains "[anything]", we strip
  // it away.  In practice this should not be fatal
  // because your variable will still show up in the tree.
  string::size_type pos1 = var.find('[');
  string::size_type pos2 = var.find(']');
  if( pos1 != string::npos && pos2 != string::npos ) {
    return var.substr(0,pos1) + var.substr(pos2+1,var.length());
//      cout << "THaOutput:WARNING:: Stripping away";
//      cout << "unwanted brackets from "<<var<<endl;
  }
  return var;
}

//_____________________________________________________________________________
std::vector<string> THaOutput::reQuote(const std::vector<string>& input) const {
  // Specialist private function needed by EPICs line parser:
  // The problem is that the line was split by white space, so
  // a line like "'RF On'=42"  must be repackaged into
  // one string, i.e. "'RF" and "On'=42" put back together.
  std::vector<string> result;
  result.clear();
  int first_quote = 1;
  int to_add = 0;
  string temp1,temp2,temp3;
  string::size_type pos1,pos2;
  for (vector<string>::const_iterator str = input.begin(); str !=
	 input.end(); ++str ) {
    temp1 = *str;
    pos1 = temp1.find("'");
    if (pos1 != string::npos) {
      if (first_quote) {
	temp2.assign(temp1.substr(pos1,temp1.length()));
// But there might be a 2nd "'" with no spaces
// like "'Yes'" (silly, but understandable & allowed)
	temp3.assign(temp1.substr(pos1+1,temp1.length()));
	pos2 = temp3.find("'");
	if (pos2 != string::npos) {
	  temp1.assign(temp3.substr(0,pos2));
	  temp2.assign(temp3.substr
		       (pos2+1,temp3.length()));
	  temp3 = temp1+temp2;
	  result.push_back(temp3);
	  continue;
	}
	first_quote = 0;
	to_add = 1;
      } else {
	temp2 = temp2 + " " + temp1;
	result.push_back(temp2);
	temp2.clear();
	first_quote = 1;
	to_add = 0;
      }
    } else {
      if (to_add) {
	temp2 = temp2 + " " + temp1;
      } else {
	result.push_back(temp1);
      }
    }
  }
  return result;
}

//_____________________________________________________________________________
string THaOutput::CleanEpicsName(const string& input) const
{
// To clean up EPICS variable names that contain
// bad characters like ":" and arithmetic operations
// that confuse TTree::Draw().
// Replace all 'badchar' with 'goodchar'

  static const char badchar[]=":+-*/=";
  static const string goodchar = "_";
  int numbad = sizeof(badchar)/sizeof(char) - 1;

  string output = input;

  for (int i = 0; i < numbad; i++) {
     string sbad(&badchar[i]);
     sbad.erase(1,sbad.size());
     string::size_type pos = input.find(sbad,0);
     while (pos != string::npos) {
       output.replace(pos,1,goodchar);
       pos = input.find(sbad,pos+1);
     }
  }
  return output;
}


//_____________________________________________________________________________
void THaOutput::ErrFile(Int_t iden, const string& sline) const
{
  // Print error messages about the output definition file.
  if (iden == -1) {
    cerr << "<THaOutput::LoadFile> WARNING: file " << sline;
    cerr << " does not exist." << endl;
    cerr << "See $ANALYZER/examples/output.def for an example.\n";
    cerr << "Output will only contain event objects "
      "(this may be all you want).\n";
    return;
  }
  if (fOpenEpics) return;  // No error
  cerr << "THaOutput::ERROR: Syntax error in output definition file."<<endl;
  cerr << "The offending line is :\n"<<sline<<endl<<endl;
  switch (iden) {
     case kVar:
       cerr << "For variables, the syntax is: "<<endl;
       cerr << "    variable  variable-name"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    variable   R.vdc.v2.nclust"<<endl;;
     case kCut:
     case kForm:
       cerr << "For formulas or cuts, the syntax is: "<<endl;
       cerr << "    formula(or cut)  formula-name  formula-expression"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    formula targetX 1.464*B.bpm4b.x-0.464*B.bpm4a.x"<<endl;
     case kH1F:
     case kH1D:
     case kvH1F:
     case kvH1D:
       cerr << "For 1D histograms, the syntax is: "<<endl;
       cerr << "  [sv]TH1F(or TH1D) name  'title' ";
       cerr << "variable nbin xlo xhi [cut-expr]"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH1F  tgtx 'target X' targetX 100 -2 2"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       cerr << "optionally can impose THaCut expression 'cut-expr'"<<endl;
       break;
     case kH2F:
     case kH2D:
     case kvH2F:
     case kvH2D:
       cerr << "For 2D histograms, the syntax is: "<<endl;
       cerr << "  [sv]TH2F(or TH2D)  name  'title'  varx  vary";
       cerr << "  nbinx xlo xhi  nbiny ylo yhi [cut-expr]"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH2F t12 't1 vs t2' D.timeroc1  D.timeroc2";
       cerr << "  100 0 20000 100 0 35000"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       cerr << "optionally can impose THaCut expression 'cut-expr'"<<endl;
       break;
     default:
       cerr << "Illegal line: " << sline << endl;
       cerr << "See the documentation or ask Bob Michaels"<<endl;
       break;
  }
}

//_____________________________________________________________________________
void THaOutput::Print() const
{
  // Printout the definitions. Amount printed depends on verbosity level,
  // set with SetDebug().

  typedef BranchMap_t::const_iterator Iter_t;

  if( fDebug > 0 ) {
    if( fBranches.empty() ) {
      ::Warning("THaOutput", "no output defined");
      return;
    }

    cout << endl << "THaOutput definitions: " << endl;

    if( fNvar > 0 ) {
      cout << "=== Number of variables " << fNvar << endl;
      if( fDebug > 1 ) {
	cout << endl;
	UInt_t i = 0;
	for (Iter_t it = fBranches.begin(); it != fBranches.end(); ++i, ++it ) {
	  if( it->second->GetType() == kVar ) {
	    cout << "Variable # " << i << " =  " << it->first << endl;
	    //TODO: add some kind of LongPrint for fDebug > 2
	  }
	}
      }
    }

    if( fNform > 0 ) {
      cout << "=== Number of formulas " << fNform << endl;
      if( fDebug > 1 ) {
	cout << endl;
	UInt_t i = 0;
	for (Iter_t it = fBranches.begin(); it != fBranches.end(); ++i, ++it ) {
	  if( it->second->GetType() == kForm ) {
	    cout << "Formula # " << i << " =  " << it->first << endl;
	    //	      if( fDebug>2 ) {
	    // TODO
	    //	      (*iform)->LongPrint();
	    //	      }
	  }
	}
      }
    }

    if( fNcut > 0 ) {
      cout << "=== Number of cuts " << fNcut <<endl;
      if( fDebug > 1 ) {
	cout << endl;
	UInt_t i = 0;
	for( Iter_t it = fBranches.begin(); it != fBranches.end(); ++i, ++it ) {
	  if( it->second->GetType() == kCut ) {
	    if( fDebug>2 ) {
	      // TODO
	      //	      (*icut)->LongPrint();
	    } else {
	      cout << "Cut # " << i << " =  " << it->first << endl;
	    }
	  }
	}
      }
    }
    if( !fHistos.empty() ) {
      cout << "=== Number of histograms "<<fHistos.size()<<endl;
      if( fDebug > 1 ) {
	cout << endl;
	UInt_t i = 0;
	for( Iterc_h_t ihist = fHistos.begin(); ihist != fHistos.end(); i++, ihist++ ) {
	  cout << "Histogram # "<<i<<endl;
	  (*ihist)->Print();
	}
      }
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
Int_t THaOutput::ParseHistogramDef( EId key, const string& sline,
				    HistogramParameters& hpar )
{
  // Parse the string that defines the histogram.
  // The title must be enclosed in single quotes (e.g. 'my title').
  // Ret value 'result' means:  -1 == error,  1 == everything ok.

  vector<string> strvect = Split(sline);

  hpar.clear();
  if( key == kInvalidEId || strvect.size() < 7 )
    return -1;
  hpar.ikey = key;
  hpar.sname = StripBracket( strvect[1] );
  string::size_type pos1 = sline.find('\'');
  string::size_type pos2 = sline.rfind('\'');
  if( pos1 == string::npos || pos2 == string::npos || pos2 <= pos1 )
    // Missing or unbalanced quotes around histogram title string
    // (does not catch cases where more than two single quotes are present)
    return -1;
  hpar.stitle = sline.substr(pos1+1,pos2-pos1-1);

  // Find the first element in strvect after the title string (which may have
  // been split at embedded whitespaces)
  vector<string>::reverse_iterator::difference_type idef = 0;
  for( vector<string>::reverse_iterator ir = strvect.rbegin();
       ir != strvect.rend(); ++ir ) {
    if( ir->find('\'') != string::npos ) {
      idef = std::distance(ir,strvect.rend());
      break;
    }
  }
  assert( idef > 0 ); // else should have quit at pos1==npos test

  vector<string>::size_type ndef = strvect.size() - idef;
  if( ndef == 4 || ndef == 5) {
    // 1D histogram
    if( key != kH1F && key != kH1D && key != kvH1F && key != kvH1D )
      return -1;
    hpar.sfvarx.swap(strvect[idef]);
    hpar.nx  = atoi( strvect[idef+1].c_str() );
    hpar.xlo = atof( strvect[idef+2].c_str() );
    hpar.xhi = atof( strvect[idef+3].c_str() );
    // Optional cut
    if( ndef == 5 ) {
      hpar.scut.swap(strvect[idef+4]);
    }
    return 1;
  }
  else if( ndef == 8 || ndef == 9 ) {
    // 2D histogram
    if( key != kH2F && key != kH2D && key != kvH2F && key != kvH2D )
      return -1;
    hpar.sfvarx.swap(strvect[idef]);
    hpar.sfvary.swap(strvect[idef+1]);
    hpar.nx  = atoi( strvect[idef+2].c_str() );
    hpar.xlo = atof( strvect[idef+3].c_str() );
    hpar.xhi = atof( strvect[idef+4].c_str() );
    hpar.ny  = atoi( strvect[idef+5].c_str() );
    hpar.ylo = atof( strvect[idef+6].c_str() );
    hpar.yhi = atof( strvect[idef+7].c_str() );
    // Optional cut
    if( ndef == 9 ) {
      hpar.scut.swap(strvect[idef+8]);
    }
    return 1;
  }
  return -1;
}

//_____________________________________________________________________________
Int_t THaOutput::BuildBlock(const string& blockn, DefinitionSet& defs)
{
  // From the block name, identify and save a specific grouping
  // of global variables by adding them to the fVarnames list.
  //
  // For efficiency, at the end of building the list we should
  // ensure that variables are listed only once.
  //
  // Eventually, we can have some specially named blocks,
  // but for now we simply will use pattern matching, such that
  //   block L.*
  // would save all variables from the left spectrometer.

  TRegexp re(blockn.c_str(),kTRUE);
  TIter next(gHaVars);
  TObject *obj;

  Int_t nvars=0;
  while ((obj = next())) {
    TString s = obj->GetName();
    if ( s.Index(re) != kNPOS ) {
      string vn(s.Data());
      AddBranchName(vn);
      defs.varnames.push_back(STDMOVE(vn));
      nvars++;
    }
  }
  return nvars;
}

//_____________________________________________________________________________
void THaOutput::SetDebug( Int_t level )
{
  // Set verbosity level for debug messages

  fDebug = level;
}

//_____________________________________________________________________________
ClassImp(THaOutput)
