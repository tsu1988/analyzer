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
#include "THaVform.h"
#include "THaVhist.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaTextvars.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaEvtTypeHandler.h"
#include "THaEpicsEvtHandler.h"
#include "THaScalerEvtHandler.h"
#include "THaString.h"
#include "THaBenchmark.h"
#include "DataBuffer.h"
#include "TROOT.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
#include "TRegexp.h"
#include "TError.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>
//#include <iterator>
#include <sstream>

using namespace std;
using namespace Podd;
using namespace THaString;

typedef vector<THaOdata*>::iterator Iter_o_t;
typedef vector<THaVform*>::iterator Iter_f_t;
typedef vector<THaVhist*>::iterator Iter_h_t;
typedef vector<string>::iterator Iter_s_t;

Int_t THaOutput::fgVerbose = 1;
//FIXME: these should be member variables
static Bool_t fgDoBench = kTRUE;
static THaBenchmark fgBench;

//_____________________________________________________________________________
struct THaOutput::HistogramParameters {
  string stitle, sfvarx, sfvary, scut;
  Double_t xlo,xhi,ylo,yhi;
  Int_t nx,ny,iscut;
};

//_____________________________________________________________________________
struct THaOutput::DefinitionSet {
  // Definitions parsed from the configuration file
  vector<string> varnames;    // Variable names
  vector<string> formnames;   // Formula names
  vector<string> formdef;     // Formula definitions
  vector<string> cutnames;    // Cut names
  vector<string> cutdef;      // Cut definitions
  void clear() {
    formnames.clear(); formdef.clear();
    cutnames.clear(); cutdef.clear();
  }
};

//_____________________________________________________________________________
class THaOutput::VariableInfo {
public:
  explicit VariableInfo( const THaVar* pvar=0 )
    : fVar(pvar), fBranch(0), fBuffer(0) {}
  VariableInfo( const VariableInfo& other );
  VariableInfo& operator=( const VariableInfo& rhs );
#if __cplusplus >= 201103L
  VariableInfo( VariableInfo&& other );
  VariableInfo& operator=( VariableInfo&& rhs);
#endif
  ~VariableInfo();
  Int_t AddBranch( const std::string& branchname, VarMap_t& varmap, TTree* fTree );
  Int_t Fill();
private:
  const THaVar*     fVar;
  TBranch*          fBranch;
  Podd::DataBuffer* fBuffer;
};

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
   pos = sdata.find("=");
   if (pos != string::npos) {
      temp1.assign(sdata.substr(0,pos));
      temp2.assign(sdata.substr(pos+1,sdata.length()));
// In case someone used "==" instead of "="
      if (temp2.find("=") != string::npos) 
            temp2.assign
              (sdata.substr(pos+2,sdata.length()));
      if (strlen(temp2.c_str()) > 0) {
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
    pos1 = input.find("'");
    if (pos1 != string::npos) {
     temp1.assign(input.substr(pos1+1,input.length()));
     pos2 = temp1.find("'");
     if (pos2 != string::npos) {
      result.assign(temp1.substr(0,pos2));
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

//_____________________________________________________________________________
THaOdata::THaOdata( const THaOdata& other )
  : tree(other.tree), name(other.name), nsize(other.nsize) 
{
  data = new Double_t[nsize]; ndata = other.ndata;
  memcpy( data, other.data, nsize*sizeof(Double_t));
}

//_____________________________________________________________________________
THaOdata& THaOdata::operator=(const THaOdata& rhs )
{ 
  if( this != &rhs ) {
    tree = rhs.tree; name = rhs.name;
    if( nsize < rhs.nsize ) {
      nsize = rhs.nsize; delete [] data; data = new Double_t[nsize];
    }
    ndata = rhs.ndata; memcpy( data, rhs.data, nsize*sizeof(Double_t));
  }
  return *this;   
}

//_____________________________________________________________________________
void THaOdata::AddBranches( TTree* _tree, string _name )
{
  name = _name;
  tree = _tree;
  string sname = "Ndata." + name;
  string leaf = sname;
  tree->Branch(sname.c_str(),&ndata,(leaf+"/I").c_str());
  // FIXME: defined this way, ROOT always thinks we are variable-size
  leaf = "data["+leaf+"]/D";
  tree->Branch(name.c_str(),data,leaf.c_str());
}

//_____________________________________________________________________________
Bool_t THaOdata::Resize(Int_t i)
{
  static const Int_t MAX = 4096;
  if( i > MAX ) return true;
  Int_t newsize = nsize;
  while ( i >= newsize ) { newsize *= 2; } 
  Double_t* tmp = new Double_t[newsize];
  memcpy( tmp, data, nsize*sizeof(Double_t) );
  delete [] data; data = tmp; nsize = newsize;
  if( tree )
    tree->SetBranchAddress( name.c_str(), data );
  return false;
}

//_____________________________________________________________________________
THaOutput::THaOutput() :
  /*fNvar(0), fVar(NULL), */fEpicsVar(0), fTree(NULL), 
   fEpicsTree(NULL), fInit(false)
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
  }
  //  if (fVar) delete [] fVar;
  if (fEpicsVar) delete [] fEpicsVar;
  for (Iter_o_t od = fOdata.begin();
       od != fOdata.end(); ++od) delete *od;
  for (Iter_f_t itf = fFormulas.begin();
       itf != fFormulas.end(); ++itf) delete *itf;
  for (Iter_f_t itf = fCuts.begin();
       itf != fCuts.end(); ++itf) delete *itf;
  for (Iter_h_t ith = fHistos.begin();
       ith != fHistos.end(); ++ith) delete *ith;
  for (vector<THaEpicsKey* >::iterator iep = fEpicsKey.begin();
       iep != fEpicsKey.end(); ++iep) delete *iep;
}

//_____________________________________________________________________________
static inline const char* GetVarTypeStr( const THaVar* pvar )
{
  // Return a string suitable for a TTree::Branch definition corresponding
  // to the type of the global variable pvar

  // TODO: support Bool_t ("O")

  switch( pvar->GetType() ) {
  case kDouble:
  case kDoubleV:
  case kDoubleM:
  case kDoubleP:
  case kDouble2P:
    return "D";
  case kFloat:
  case kFloatV:
  case kFloatM:
  case kFloatP:
  case kFloat2P:
    return "F";
  case kLong:
  case kLongP:
  case kLong2P:
    // Long/ULong are defined as (U)Long64_t in the global variable system
    return "L";
  case kULong:
  case kULongP:
  case kULong2P:
    return "l";
  case kInt:
  case kIntV:
  case kIntM:
  case kIntP:
  case kInt2P:
    return "I";
  case kUInt:
  case kUIntV:
  case kUIntP:
  case kUInt2P: 
    return "i";
  case kShort:
  case kShortP:
  case kShort2P:
    return "S";
  case kUShort: 
  case kUShortP:
  case kUShort2P:
    return "s";
  case kChar:
  case kCharP: // TODO: maybe "C"?
  case kChar2P:
    return "B";
  case kByte:
  case kByteP:
  case kByte2P:
    return "b";

  //TODO
  case kObject:
    break;
  case kTString:
    break;
  case kString:
    break;
  case kObjectP:
    break;
  case kObject2P:
    break;
  case kVarTypeEnd:
    assert(false); // Should never happen, undefined type
    break;
  }
  return "";
}

//_____________________________________________________________________________
THaOutput::VariableInfo::VariableInfo( const VariableInfo& rhs )
  : fVar(rhs.fVar), fBranch(rhs.fBranch), fBuffer(0)
{
  // Copy constructor
  if( rhs.fBuffer )
    fBuffer = new DataBuffer(*rhs.fBuffer);
}

//_____________________________________________________________________________
THaOutput::VariableInfo&
THaOutput::VariableInfo::operator=( const VariableInfo& rhs)
{
  // Assignment operator
  if( this != &rhs ) {
    fVar = rhs.fVar;
    fBranch = rhs.fBranch;
    delete fBuffer;
    if( rhs.fBuffer )
      fBuffer = new DataBuffer(*rhs.fBuffer);
    else
      fBuffer = 0;
  }
  return *this;
}

#if __cplusplus >= 201103L
//_____________________________________________________________________________
THaOutput::VariableInfo::VariableInfo( VariableInfo&& rhs )
  : fVar(rhs.fVar), fBranch(rhs.fBranch), fBuffer(rhs.fBuffer)
{
  // Move constructor
  rhs.fVar = nullptr;
  rhs.fBranch = nullptr;
  rhs.fBuffer = nullptr;
}

//_____________________________________________________________________________
THaOutput::VariableInfo&
THaOutput::VariableInfo::operator=( VariableInfo&& rhs)
{
  // Move assignment
  if( this != &rhs ) {
    delete fBuffer;
    fVar = rhs.fVar;
    fBranch = rhs.fBranch;
    fBuffer = rhs.fBuffer;
    rhs.fVar = nullptr;
    rhs.fBranch = nullptr;
    rhs.fBuffer = nullptr;
  }
  return *this;
}
#endif

//_____________________________________________________________________________
THaOutput::VariableInfo::~VariableInfo()
{
  delete fBuffer;
}

//_____________________________________________________________________________
Int_t THaOutput::VariableInfo::AddBranch( const string& branchname,
					  VarMap_t& varmap, TTree* tree )
{
  assert( !fBranch );   // should never be called twice
  if( fBranch )
    return -1;

  // Is this an object that can be streamed directly?
  if( fVar->IsStreamable() ) {
    VarType type = fVar->GetType();
    if( fVar->IsVector() ) {
      // std::vector of certain basic types
      void* ptr = const_cast<void*>(fVar->GetValuePointer());
      switch( type ) {
      case kIntV:
	fBranch = tree->Branch( branchname.c_str(), static_cast<vector<int>*>(ptr) );
	break;
      case kUIntV:
	fBranch = tree->Branch( branchname.c_str(), static_cast<vector<unsigned int>*>(ptr) );
	break;
      case kFloatV:
	fBranch = tree->Branch( branchname.c_str(), static_cast<vector<float>*>(ptr) );
	break;
      case kDoubleV:
	fBranch = tree->Branch( branchname.c_str(), static_cast<vector<double>*>(ptr) );
	break;
      default:
	assert(false);  // unsupported vector type
	return -2;
      }
      return 0;
    }
    else if( type == kObject2P ) {
      // Stream objects deriving from TObject directly using the class name interface
      void* ptr = const_cast<void*>(fVar->GetValuePointer());
      if( !ptr ) {
	cerr << "Zero pointer for global variable object " << fVar->GetName()
	     << ", ignoring definition" << endl;
	return 1;
      }
      string branchname_dot = branchname + '.';
      fBranch = tree->Branch( branchname_dot.c_str(),
			      (*static_cast<TObject**>(ptr))->ClassName(), ptr );
      return 0;
    }
    cerr << "Unsupported streamable object type = \"" << fVar->GetTypeName()
	 << "\" for variable " << fVar->GetName() << ", ignoring definition"
	 << endl;
    return 1;
  }

  // Basic data
  string typec = GetVarTypeStr( fVar );
  if( typec.empty() ) {
    cerr << "Unsupported variable type " << fVar->GetTypeName()
	 << "for global variable " << fVar->GetName()
	 << ", ignoring definition" << endl;
    return 1;
  }

  string leafdef;
  if( fVar->IsVarArray() ) {
    string countbranch;
    if( fVar->HasSizeVar() ) {
      // If this variable has a counter variable, check if it is available as a
      // global variable already, and if so, use it. Otherwise, set up an
      // individual counter branch for this variable-size array
      const Int_t* pcounter = fVar->GetDim();
      assert( pcounter );  // else HasSizeVar() lies
      TIter next(gHaVars); // FIXME: use local variable list
      THaVar* pvar;
      while( (pvar = static_cast<THaVar*>(next())) ) {
	if( pvar == fVar ) // can't be counter for itself
	  continue;
	if( pvar->GetType() == kInt && pvar->GetValuePointer() == pcounter ) {
	  countbranch = pvar->GetName();
	  VarMap_t::iterator it = varmap.find( countbranch );
	  if( it == varmap.end() ) {
	    pair<VarMap_t::iterator,bool> ins =
	      varmap.insert( make_pair(countbranch,VariableInfo(pvar)) );
	    assert( ins.second );
	    VariableInfo& varinfo = ins.first->second;
	    Int_t ret = varinfo.AddBranch( countbranch, varmap, tree );
	    if( ret ) return ret;
	  }
	  break;
	}
      }
    }
    else {
      // For all other variable-size arrays, try to find other variables that
      // report being parallel to this one
      TIter next(gHaVars); // FIXME: use local variable list
      THaVar* pvar;
      while( (pvar = static_cast<THaVar*>(next())) ) {
	if( fVar == pvar )
	  // fVar is the first variable with this reported size: set up
	  // a new count branch. Just fall through to let this happen.
	  break;
	if( fVar->HasSameSize(pvar) ) {
	  // Found another variable before the current one, so the count branch
	  // must already exist. Find it and indicate so in the leafdef.
	  //FIXME: relies on fVar->GetName() == branchname,  which is true,
	  // but the code doesn't reflect it
	  countbranch = "Ndata.";
	  countbranch.append( pvar->GetName() );
	  break;
	}
      }
      assert( pvar != 0 ); // fVar must be in gHaVars
    }
    if( countbranch.empty() ) {
      countbranch = "Ndata." + branchname;
      tree->Branch( countbranch.c_str(), (void*)fVar->GetDim(), (countbranch+"/I").c_str() );
    }
    leafdef = branchname + '[' + countbranch + ']';

  } else if( fVar->IsArray() ) {
    stringstream ostr;
    assert( fVar->GetNdim() > 0 );  // else IsArray() lies
    if( fVar->GetNdim() != 2 ) {  // flatten multidimensional arrays ( d>2 )
      ostr << '[' << fVar->GetLen() << ']';
    } else {
      ostr << '[' << fVar->GetDim()[0] << "][" << fVar->GetDim()[1] << ']';
    }
    leafdef = branchname + ostr.str();
  } else {
    leafdef = branchname;
  }
  leafdef += "/" + typec;

  // Now for the data pointer. This is quite tricky
  //
  // Variable type    Example             Array  VarSz  Address behavior
  // ------------------------------------------------------------------------------
  // Member variable  int fData           no     no     avail, fixed
  // Member array     int fData[10]       yes    no     avail, fixed
  // Member pointer   int *fData          yes    yes    avail, may realloc on re-init
  // Non-array handle int **fData         no     no     avail, may realloc per event
  // std::vector      vector<int> fData   yes    yes    stream directly
  // ROOT object      TLorentzVector      no     no     stream directly
  // TSeqCollection   TClonesArray elem   yes    yes    must copy -> use buffer addr
  // Pointer array    **fData             yes    yes    must copy -> use buffer addr
  // Method call      GetX()              no     no     must copy -> use buffer addr

  const void* leafp;
  if( fVar->IsContiguous() ) {
    leafp = fVar->GetDataPointer();
  } else {
    // We're dealing with non-contiguous data (array of pointers to data
    // or members of a TSeqCollection/TClonesArray), or with transient
    // data (method call). In these cases, we need to store a copy of 
    // the data locally since tree branches expect contiguous data.
    size_t len = fVar->GetTypeSize();
    if( fVar->IsVarArray() )
      len *= 8;
    fBuffer = new DataBuffer(len);

    leafp = fBuffer->Get();  // the branch address will be reset later if necessary
  }
  fBranch = tree->Branch( branchname.c_str(), const_cast<void*>(leafp), leafdef.c_str() );
    
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::VariableInfo::Fill()
{
  // For non-contiguous data, fill the local buffer. Update branch address for
  // those variable types for which it might vary

  // Get the current variable length. For 1-d arrays, get it via GetDim() to
  // update the internal counter that a count branch might point to
  Int_t len = fVar->GetNdim();
  if( len == 0 )
    len = 1;
  else if( len == 1 )
    len = *fVar->GetDim();
  else
    len = fVar->GetLen();

  if( len == 0 )
    return 0;

  if( fVar->IsBasic() && !fVar->IsPointerArray() )
    return 0;

  void* ptr = 0;
  if( fBuffer ) {
    // Non-contiguous variable-size arrays (e.g. pointer array, SeqCollection)
    size_t siz = fVar->GetTypeSize();
    fBuffer->Clear();
    fBuffer->Resize( siz*len );
    for( Int_t i = 0; i < len; ++i )
      fBuffer->Fill( fVar->GetDataPointer(i), siz, i );
    // Fill() may reallocate the buffer
    ptr = fBuffer->Get();
  }
  //TODO:
  // else if( fVar->GetType() == kObject2P )
  //   ptr = const_cast<void*>( fVar->GetValuePointer() );
  else
    // Contiguous non-basic data (vector, MethodVar)
    ptr = const_cast<void*>( fVar->GetDataPointer() );
  assert(ptr);

  if( fBranch->GetAddress() != ptr )
    fBranch->SetAddress(ptr);

  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::Init( const char* filename )
{
  // Initialize output system. Required before anything can happen.
  //
  // Only re-attach to variables and re-compile cuts and formulas
  // upon re-initialization (eg: continuing analysis with another file)
  if( fInit ) {
    cout << "\nTHaOutput::Init: Info: THaOutput cannot be completely"
	 << " re-initialized. Keeping existing definitions." << endl;
    cout << "Global Variables are being re-attached and formula/cuts"
	 << " are being re-compiled." << endl;
    
    // Assign pointers and recompile stuff reliant on pointers.

    if ( Attach() ) return -4;

    Print();

    return 1;
  }

  if( !gHaVars ) return -2;

  if( fgDoBench ) fgBench.Begin("Init");

  DefinitionSet defs;
  Int_t err = LoadFile( filename, defs );
  if( err != -1 && err != 0 ) {
    if( fgDoBench ) fgBench.Stop("Init");
    return -3;
  }

  fTree = new TTree("T","Hall A Analyzer Output DST");
  fTree->SetAutoSave(200000000);
  fOpenEpics  = kFALSE;
  fFirstEpics = kTRUE; 

  if( err == -1 ) {
    if( fgDoBench ) fgBench.Stop("Init");
    return 0;       // No error if file not found, but please
  }                    // read the instructions.

  //  fNvar = defs.varnames.size();  // this gets reassigned below

  // Variable definitions
  for( Iter_s_t it = defs.varnames.begin(); it != defs.varnames.end(); ++it ) {
    const string& branchname = *it;
    const THaVar* pvar = gHaVars->Find(branchname.c_str());
    if (!pvar) {
      cout << endl << "THaOutput::Init: WARNING: Global variable ";
      cout << branchname << " does not exist. "<< endl;
      cout << "There is probably a typo error... "<<endl;
      continue;
    }
    pair<VarMap_t::iterator,bool> ins =
      fVariables.insert( make_pair(branchname,VariableInfo(pvar)) );
    if( ins.second ) {
      VarMap_t::iterator item = ins.first;
      VariableInfo& vinfo = item->second;
      Int_t ret = vinfo.AddBranch( branchname, fVariables, fTree );
      if( ret != 0 ) {
	// Error message already printed
	fVariables.erase( item );
	continue;
      }
      fVNames.push_back(branchname);
    }
  }

  // Formula definitions
  Int_t k = 0;
  for (Iter_s_t inam = defs.formnames.begin(); inam != defs.formnames.end(); ++inam, ++k) {
    string tinfo = Form("f%d",k);
    // FIXME: avoid duplicate formulas
    THaVform* pform = new THaVform("formula",inam->c_str(),defs.formdef[k].c_str());
    Int_t status = pform->Init();
    if ( status != 0) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << *inam << endl;
      cout << "There is probably a typo error... " << endl;
      pform->ErrPrint(status);
      delete pform;
      --k;
      continue;
    }
    pform->SetOutput(fTree);
    fFormulas.push_back(pform);
    if( fgVerbose > 2 )
      pform->LongPrint();  // for debug
// Add variables (i.e. those var's used by the formula) to tree.
// Reason is that TTree::Draw() may otherwise fail with ERROR 26 
    vector<string> avar = pform->GetVars();
    for (Iter_s_t it = avar.begin(); it != avar.end(); ++it) {
      string svar = StripBracket(*it);
      THaVar* pvar = gHaVars->Find(svar.c_str());
      if (pvar) {
	if (pvar->IsArray()) {
	  Iter_s_t it = find(fArrayNames.begin(),fArrayNames.end(),svar);
	  if( it == fArrayNames.end() ) {
	    fArrayNames.push_back(svar);
	    fOdata.push_back(new THaOdata());
	  }
	} else {
	  //FIXME: search fVariables here
	  Iter_s_t it = find(fVNames.begin(),fVNames.end(),svar);
	  if( it == fVNames.end() )
	    fVNames.push_back(svar);
	}
      }
      //FIXME: what if !pvar ... ?
    }
  }

  // Add fOdata branches to the tree
  k = 0;
  for(Iter_o_t iodat = fOdata.begin(); iodat != fOdata.end(); ++iodat, ++k)
    (*iodat)->AddBranches(fTree, fArrayNames[k]);

  // superseded by VariableInfo
  // fNvar = fVNames.size();
  // fVar = new Double_t[fNvar];
  // for (Int_t k = 0; k < fNvar; ++k) {
  //   string tinfo = fVNames[k] + "/D";
  //   fTree->Branch(fVNames[k].c_str(), &fVar[k], tinfo.c_str(), kNbout);
  // }

  // Cuts initialization
  k = 0;
  for (Iter_s_t inam = defs.cutnames.begin(); inam != defs.cutnames.end(); ++inam, ++k ) {
    // FIXME: avoid duplicate cuts
    THaVform* pcut = new THaVform("cut", inam->c_str(), defs.cutdef[k].c_str());
    Int_t status = pcut->Init();
    if ( status != 0 ) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << *inam << endl;
      cout << "There is probably a typo error... " << endl;
      pcut->ErrPrint(status);
      delete pcut;
      --k;
      continue;
    }
    pcut->SetOutput(fTree);
    fCuts.push_back(pcut);
    if( fgVerbose>2 )
      pcut->LongPrint();  // for debug
  }

  // Ensure that the variables referenced in the histograms and cuts exist in the tree
  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist) {
// After initializing formulas and cuts, must sort through
// histograms and potentially reassign variables.  
// A histogram variable or cut is either a string (which can 
// encode a formula) or an externally defined THaVform. 
    string sfvarx = (*ihist)->GetVarX();
    string sfvary = (*ihist)->GetVarY();
    for (Iter_f_t iform = fFormulas.begin(); iform != fFormulas.end(); ++iform) {
      string stemp((*iform)->GetName());
      if (CmpNoCase(sfvarx,stemp) == 0) { 
	(*ihist)->SetX(*iform);
      }
      if (CmpNoCase(sfvary,stemp) == 0) { 
	(*ihist)->SetY(*iform);
      }
    }
    if ((*ihist)->HasCut()) {
      string scut = (*ihist)->GetCutStr();
      for (Iter_f_t icut = fCuts.begin(); icut != fCuts.end(); ++icut) {
        string stemp((*icut)->GetName());
        if (CmpNoCase(scut,stemp) == 0) { 
	  (*ihist)->SetCut(*icut);
        }
      }
    }
    (*ihist)->Init();
  }

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
  // 	fVariables[ivar] = pvar;
  //     } else {
  // 	cout << "\tTHaOutput::Attach: ERROR: Global variable " << fVNames[ivar]
  // 	     << " changed from simple to array!! Leaving empty space for variable"
  // 	     << endl;
  // 	fVariables[ivar] = 0;
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
  // 	fArrays[ivar] = pvar;
  //     } else {
  // 	cout << "\tTHaOutput::Attach: ERROR: Global variable " << fVNames[ivar]
  // 	     << " changed from ARRAY to Simple!! Leaving empty space for variable"
  // 	     << endl;
  // 	fArrays[ivar] = 0;
  //     }
  //   } else {
  //     cout << "\nTHaOutput::Attach: WARNING: Global variable ";
  //     cout << fVarnames[ivar] << " NO LONGER exists (it did before). "<< endl;
  //     cout << "This is not supposed to happen... "<<endl;
  //   }
  // }

  // Reattach formulas, cuts, histos

  for (Iter_f_t iform=fFormulas.begin(); iform!=fFormulas.end(); ++iform) {
    (*iform)->ReAttach();
  }

  for (Iter_f_t icut=fCuts.begin(); icut!=fCuts.end(); ++icut) {
    (*icut)->ReAttach(); 
  }

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

  if( fgDoBench ) fgBench.Begin("Formulas");
  for (Iter_f_t iform = fFormulas.begin(); iform != fFormulas.end(); ++iform)
    if (*iform) (*iform)->Process();
  if( fgDoBench ) fgBench.Stop("Formulas");

  if( fgDoBench ) fgBench.Begin("Cuts");
  for (Iter_f_t icut = fCuts.begin(); icut != fCuts.end(); ++icut)
    if (*icut) (*icut)->Process();
  if( fgDoBench ) fgBench.Stop("Cuts");

  if( fgDoBench ) fgBench.Begin("Variables");
  for( VarMap_t::iterator it = fVariables.begin(); it != fVariables.end(); ++it ) {
    VariableInfo& vinfo = it->second;
    vinfo.Fill();
  }
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
  // 	if( fgVerbose>0 && first ) {
  // 	  cerr << "THaOutput::ERROR: storing too much variable sized data: " 
  // 	       << pvar->GetName() <<"  "<<pvar->GetLen()<<endl;
  // 	  first = false;
  // 	}
  //     }
  //   }
  // }
  if( fgDoBench ) fgBench.Stop("Variables");

  if( fgDoBench ) fgBench.Begin("Histos");
  for ( Iter_h_t it = fHistos.begin(); it != fHistos.end(); ++it )
    (*it)->Process();
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
    fgBench.Print("Variables");
    fgBench.Print("Formulas");
    fgBench.Print("Cuts");
    fgBench.Print("Histos");
    fgBench.Print("TreeFill");
    fgBench.Print("EPICS");
    fgBench.Print("End");
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::LoadFile( const char* filename, DefinitionSet& defs ) 
{
  // Process the file that defines the output
  
  if( !filename || !*filename || strspn(filename," ") == strlen(filename) ) {
    ::Error( "THaOutput::LoadFile", "invalid file name, "
	     "no output definition loaded" );
    return -1;
  }
  string loadfile(filename);
  ifstream odef(loadfile.c_str());
  if ( !odef ) {
    ErrFile(-1, loadfile);
    return -1;
  }
  const string comment = "#";
  const string whitespace( " \t" );
  string::size_type pos;
  vector<string> strvect;
  string sline;
  while (getline(odef,sline)) {
    // Blank line or comment line?
    if( sline.empty()
	|| (pos = sline.find_first_not_of( whitespace )) == string::npos
	|| comment.find(sline[pos]) != string::npos )
      continue;
    // Get rid of trailing comments
    if( (pos = sline.find_first_of( comment )) != string::npos )
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
	// std::map automatically prevents duplicate definitions
	defs.varnames.push_back(sname);
	break;
      case kForm:
	if (strvect.size() < 3) {
	  ErrFile(ikey, str);
	  continue;
	}
	defs.formnames.push_back(sname);
	defs.formdef.push_back(strvect[2]);
	break;
      case kCut:
	if (strvect.size() < 3) {
	  ErrFile(ikey, str);
	  continue;
	}
	defs.cutnames.push_back(sname);
	defs.cutdef.push_back(strvect[2]);
	break;
      case kH1f:
      case kH1d:
      case kH2f:
      case kH2d:
	{
	  // FIXME: move this into Init
	  HistogramParameters hpar;
	  if( ParseHistogramDef(ikey, str, hpar) != 1) {
	    ErrFile(ikey, str);
	    continue;
	  }
	  fHistos.push_back( new THaVhist(strvect[0],sname,hpar.stitle));
	  // Tentatively assign variables and cuts as strings. 
	  // Later will check if they are actually THaVform's.
	  fHistos.back()->SetX(hpar.nx, hpar.xlo, hpar.xhi, hpar.sfvarx);
	  if (ikey == kH2f || ikey == kH2d) {
	    fHistos.back()->SetY(hpar.ny, hpar.ylo, hpar.yhi, hpar.sfvary);
	  }
	  if (hpar.iscut != fgNocut) fHistos.back()->SetCut(hpar.scut);
	}
	break;
      case kBlock:
	// Do not strip brackets for block regexps: use strvect[1] not sname
	if( BuildBlock(strvect[1],defs) == 0 ) {
	  cout << "\nTHaOutput::Init: WARNING: Block ";
	  cout << strvect[1] << " does not match any variables. " << endl;
	  cout << "There is probably a typo error... "<<endl;
	}
	break;
      case kBegin:
      case kEnd:
	break;
      case kInvalidEId:
	cout << "Warning: keyword "<<strvect[0]<<" undefined "<<endl;
      }
    }
  }

  return 0;
}

//_____________________________________________________________________________
THaOutput::EId THaOutput::GetKeyID(const string& key) const
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
    { "th1f",     kH1f },
    { "th1d",     kH1d },
    { "th2f",     kH2f },
    { "th2d",     kH2d },
    { "block",    kBlock },
    { "begin",    kBegin },
    { "end",      kEnd },
    { 0 }
  };

  if( const KeyMap* it = keymap ) {
    while( it->name ) {
      if( CmpNoCase( key, it->name ) == 0 )
	return it->keyval;
      it++;
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
  string::size_type pos1,pos2;
  string open_brack("[");
  string close_brack("]");
  string result;
  pos1 = var.find(open_brack,0);
  pos2 = var.find(close_brack,0);
  if ((pos1 != string::npos) &&
      (pos2 != string::npos)) {
      result = var.substr(0,pos1);
      result += var.substr(pos2+1,var.length());    
//      cout << "THaOutput:WARNING:: Stripping away";
//      cout << "unwanted brackets from "<<var<<endl;
  } else {
    result = var;
  }
  return result;
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
     case kH1f:
     case kH1d:
       cerr << "For 1D histograms, the syntax is: "<<endl;
       cerr << "  TH1F(or TH1D) name  'title' ";
       cerr << "variable nbin xlo xhi [cut-expr]"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH1F  tgtx 'target X' targetX 100 -2 2"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       cerr << "optionally can impose THaCut expression 'cut-expr'"<<endl;
       break;
     case kH2f:
     case kH2d:
       cerr << "For 2D histograms, the syntax is: "<<endl;
       cerr << "  TH2F(or TH2D)  name  'title'  varx  vary";
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
  // Printout the definitions. Amount printed depends on verbosity
  // level, set with SetVerbosity().

  typedef VarMap_t::const_iterator Iterc_v_t;
  typedef vector<THaVform*>::const_iterator Iterc_f_t;
  typedef vector<THaVhist*>::const_iterator Iterc_h_t;

  if( fgVerbose > 0 ) {
    if( fVariables.size() == 0 && fFormulas.size() == 0 &&
	fCuts.size() == 0 && fHistos.size() == 0 ) {
      ::Warning("THaOutput", "no output defined");
    } else {
      cout << endl << "THaOutput definitions: " << endl;
      if( !fVariables.empty() ) {
	cout << "=== Number of variables "<<fVariables.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  
	  UInt_t i = 0;
	  for (Iterc_v_t ivar = fVariables.begin(); ivar != fVariables.end(); 
	       i++, ivar++ ) {
	    cout << "Variable # "<<i<<" =  "<< ivar->first <<endl;
	  }
	}
      }
      if( !fFormulas.empty() ) {
	cout << "=== Number of formulas "<<fFormulas.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  UInt_t i = 0;
	  for (Iterc_f_t iform = fFormulas.begin(); 
	       iform != fFormulas.end(); i++, iform++ ) {
	    cout << "Formula # "<<i<<endl;
	    if( fgVerbose>2 )
	      (*iform)->LongPrint();
	    else
	      (*iform)->ShortPrint();
	  }
	}
      }
      if( !fCuts.empty() ) {
	cout << "=== Number of cuts "<<fCuts.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  UInt_t i = 0;
	  for (Iterc_f_t icut = fCuts.begin(); icut != fCuts.end();
	       i++, icut++ ) {
	    cout << "Cut # "<<i<<endl;
	    if( fgVerbose>2 )
	      (*icut)->LongPrint();
	    else
	      (*icut)->ShortPrint();
	  }
	}
      }
      if( !fHistos.empty() ) {
	cout << "=== Number of histograms "<<fHistos.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  UInt_t i = 0;
	  for (Iterc_h_t ihist = fHistos.begin(); ihist != fHistos.end(); 
	       i++, ihist++) {
	    cout << "Histogram # "<<i<<endl;
	    (*ihist)->Print();
	  }
	}
      }
      cout << endl;
    }
  }
}

//_____________________________________________________________________________
Int_t THaOutput::ParseHistogramDef( Int_t iden, const string& sline,
				    HistogramParameters& hpar )
{
// Parse the string that defines the histogram.  
// The title must be enclosed in single quotes (e.g. 'my title').  
// Ret value 'result' means:  -1 == error,  1 == everything ok.
  Int_t result = -1;
  hpar.stitle = "";   hpar.sfvarx = "";  hpar.sfvary  = "";
  hpar.iscut = fgNocut;  hpar.scut = "";
  hpar.nx = 0; hpar.ny = 0; hpar.xlo = 0; hpar.xhi = 0; hpar.ylo = 0; hpar.yhi = 0;
  string::size_type pos1 = sline.find_first_of("'");
  string::size_type pos2 = sline.find_last_of("'");
  if (pos1 != string::npos && pos2 > pos1) {
    hpar.stitle = sline.substr(pos1+1,pos2-pos1-1);
  }
  string ctemp = sline.substr(pos2+1,sline.size()-pos2);
  vector<string> stemp = Split(ctemp);
  if (stemp.size() > 1) {
     hpar.sfvarx = stemp[0];
     Int_t ssize = stemp.size();
     if (ssize == 4 || ssize == 5) {
       sscanf(stemp[1].c_str(),"%8d",&hpar.nx);
       sscanf(stemp[2].c_str(),"%16lf",&hpar.xlo);
       sscanf(stemp[3].c_str(),"%16lf",&hpar.xhi);
       if (ssize == 5) {
         hpar.iscut = 1; hpar.scut = stemp[4];
       }
       result = 1;
     }
     if (ssize == 8 || ssize == 9) {
       hpar.sfvary = stemp[1];
       sscanf(stemp[2].c_str(),"%8d",&hpar.nx);
       sscanf(stemp[3].c_str(),"%16lf",&hpar.xlo);
       sscanf(stemp[4].c_str(),"%16lf",&hpar.xhi);
       sscanf(stemp[5].c_str(),"%8d",&hpar.ny);
       sscanf(stemp[6].c_str(),"%16lf",&hpar.ylo);
       sscanf(stemp[7].c_str(),"%16lf",&hpar.yhi);
       if (ssize == 9) {
         hpar.iscut = 1; hpar.scut = stemp[8]; 
       }
       result = 2;
     }
  }
  if (result != 1 && result != 2) return -1;
  if ((iden == kH1f || iden == kH1d) && 
       result == 1) return 1;  // ok
  if ((iden == kH2f || iden == kH2d) && 
       result == 2) return 1;  // ok
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
      defs.varnames.push_back(vn);
      nvars++;
    }
  }
  return nvars;
}

//_____________________________________________________________________________
void THaOutput::SetVerbosity( Int_t level )
{
  // Set verbosity level for debug messages

  fgVerbose = level;
}

//_____________________________________________________________________________
//ClassImp(THaOdata)
ClassImp(THaOutput)
