//*-- Author :    Ole Hansen   01-Feb-2017

//////////////////////////////////////////////////////////////////////////
//
// Output::VariableHandler
// Output::FormulaHandler
//
// Helper classes for THaOutput
//
//////////////////////////////////////////////////////////////////////////

#include "BranchHandlers.h"
#include "DataBuffer.h"
#include "HistogramAxis.h"
#include "TTree.h"
#include "TError.h"
#include "THaGlobals.h"
#include "THaVar.h"
#include "THaFormula.h"
#include "THaVarList.h"
#include <cassert>
#include <sstream>

using namespace std;
using namespace Podd;

namespace Output {

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
BranchHandler::BranchHandler( const BranchHandler& rhs )
  : fType(rhs.fType), fBranch(rhs.fBranch), fBuffer(0)
{
  // Copy constructor
  if( rhs.fBuffer )
    fBuffer = new Podd::DataBuffer(*rhs.fBuffer);
}

//_____________________________________________________________________________
BranchHandler& BranchHandler::operator=( const BranchHandler& rhs )
{
  // Assignment operator
  if( this != &rhs ) {
    fType   = rhs.fType;
    fBranch = rhs.fBranch;
    delete fBuffer;
    if( rhs.fBuffer )
      fBuffer = new Podd::DataBuffer(*rhs.fBuffer);
    else
      fBuffer = 0;
  }
  return *this;
}

#if __cplusplus >= 201103L
//_____________________________________________________________________________
BranchHandler::BranchHandler( BranchHandler&& rhs )
  : fType(rhs.fType), fBranch(rhs.fBranch), fBuffer(rhs.fBuffer)
{
  // Move constructor
  rhs.fType   = kInvalidEId;
  rhs.fBranch = nullptr;
  rhs.fBuffer = nullptr;
}

//_____________________________________________________________________________
BranchHandler& BranchHandler::operator=( BranchHandler&& rhs)
{
  // Move assignment
  if( this != &rhs ) {
    delete fBuffer;
    fType   = rhs.fType;
    fBranch = rhs.fBranch;
    fBuffer = rhs.fBuffer;
    rhs.fType   = kInvalidEId;
    rhs.fBranch = nullptr;
    rhs.fBuffer = nullptr;
  }
  return *this;
}
#endif

//_____________________________________________________________________________
BranchHandler::~BranchHandler()
{
  delete fBuffer;
}

//_____________________________________________________________________________
VariableHandler::VariableHandler( const THaVar* pvar, const THaVarList* plst,
				  EId type )
  : BranchHandler(type), fVar(pvar), fVarList(plst)
{
  assert(pvar && plst);
  fName = pvar->GetName();
}

//_____________________________________________________________________________
Int_t VariableHandler::AddBranch( css_t& branchname,
				  BranchMap_t& branches, TTree* tree )
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
	  BranchMap_t::iterator it = branches.find( countbranch );
	  if( it == branches.end() ) {
	    BranchHandler* vinfo = new VariableHandler(pvar,gHaVars);
	    Int_t ret = vinfo->AddBranch( countbranch, branches, tree );
	    if( ret ) {
	      delete vinfo;
	      return ret;
	    }
#ifndef NDEBUG
	    pair<BranchMap_t::iterator,bool> ins =
#endif
	      branches.insert( make_pair(countbranch,vinfo) );
	    assert( ins.second );
	  } else if( !dynamic_cast<VariableHandler*>(it->second) ) {
	    cerr << "Counter branch for variable " << fVar->GetName()
		 << " exists but is not a variable." << endl
		 << "Check for conflicting definitions." << endl;
	    return 1;
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
    fBuffer = new Podd::DataBuffer(len);

    leafp = fBuffer->Get();  // the branch address will be reset later if necessary
  }
  fBranch = tree->Branch( branchname.c_str(), const_cast<void*>(leafp), leafdef.c_str() );

  return 0;
}

//_____________________________________________________________________________
void VariableHandler::Attach()
{
  fVar = fVarList->Find(fName.c_str());
  //TODO: throw exception if variable not found?
  assert( fVar );
}

//_____________________________________________________________________________
Int_t VariableHandler::Fill()
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
    size_t nbytes = fVar->GetTypeSize() * len;
    fBuffer->Clear();
    fBuffer->Reserve( nbytes );
    assert( fBuffer->GetAllocation() >= nbytes );
    ptr = fBuffer->Get();
#ifndef NDEBUG
    size_t ncopied =
#endif
    fVar->GetData( ptr );
    assert( ncopied == nbytes );
    fBuffer->SetSize( nbytes );
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
Int_t VariableHandler::LinkTo( HistogramAxis& axis )
{
  return axis.Init( fVar, fVarList );
}

//_____________________________________________________________________________
Int_t FormulaHandler::AddBranch( css_t& branchname,
				 BranchMap_t& /*branches*/, TTree* tree )
{
  assert( !fBranch );   // should never be called twice
  if( fBranch )
    return -1;

  string leafdef(branchname);
  const void* leafp;
  if( fFormula->IsArray() ) {
    string countbranch;
    size_t len = sizeof(Double_t);
    if( fFormula->IsVarArray() ) {
      // Variable array

      // Try to find other formulas or variables that report being parallel to us
      //TODO

      if( countbranch.empty() ) {
	countbranch = "Ndata." + branchname;
	tree->Branch( countbranch.c_str(), (void*)&fNdata, (countbranch+"/I").c_str() );
      }
      leafdef += '[' + countbranch + ']';

      // Reserve initial space for 8 elements
      len *= 8;
    } else {
      // Fixed array
      stringstream ostr;
      Int_t ndata = fFormula->GetNdata();
      ostr << '[' << ndata << ']';
      leafdef += ostr.str();
      len *= ndata;
    }
    assert( fBuffer == 0 );
    fBuffer = new Podd::DataBuffer(len);
    leafp = fBuffer->Get();  // the branch address will be reset later if necessary

  } else {
    // Scaler
    leafdef = branchname;
    leafp = &fData;
  }

  leafdef += "/D";  // Formulas currently only support Double_t results

  fBranch = tree->Branch( branchname.c_str(), const_cast<void*>(leafp), leafdef.c_str() );

  return 0;
}

//_____________________________________________________________________________
void FormulaHandler::Attach()
{
  fFormula->Compile();
}

//_____________________________________________________________________________
Int_t FormulaHandler::Fill()
{
  // Evaluate the formula and fill the branch buffer with the result(s)
  // Update branch address if necessary

  if( fBuffer ) {
    assert( fFormula->IsArray() );
    Int_t len = fFormula->GetNdata();
    size_t nbytes = sizeof(Double_t) * len;
    fBuffer->Clear();
    fBuffer->Reserve( nbytes );
    assert( fBuffer->GetAllocation() >= nbytes );
    for( Int_t i=0; i<len; ++i ) {
      Double_t data = fFormula->EvalInstance(i);
      fBuffer->Fill( &data, sizeof(data), i );
    }
    fBuffer->SetSize( nbytes );

    void* ptr = fBuffer->Get();
    assert(ptr);
    if( fBranch->GetAddress() != ptr )
      fBranch->SetAddress(ptr);

  } else {
    fData = fFormula->Eval();
  }

  return 0;
}

//_____________________________________________________________________________
Int_t BranchHandlerFactory::AddBranch( css_t& branchname,
				       css_t& definition )
{
  // Create a branch handler, associate it with a tree branch,
  // and store it in the list of branches

  BranchMap_t::const_iterator vt = fBranches.find(branchname);
  if( vt == fBranches.end() ) {
    // New branch
    BranchHandler* handler = Create(branchname,definition);
    if( !handler ||
	handler->AddBranch(branchname,fBranches,fTree) != 0 ) {
      // Error message already printed
      delete handler;
      return -1;
    }
#ifndef NDEBUG
    pair<BranchMap_t::iterator,bool> ins =
#endif
      fBranches.insert( make_pair(branchname,handler) );
    assert( ins.second ); // else find() failed
  }
  else {
    if( IsEqual(branchname,definition,vt->second) ) {
      // if( fDebug ) ?
      ostringstream ostr;
      ostr << "Skipping duplicate but identical output definition " << branchname;
      ::Warning( fHere, "%s", ostr.str().c_str() );
      return 1;
    } else {
      ostringstream ostr;
      ostr << "Output branch " << branchname << " defined in two different ways. "
	   << "Probably a name clash in the output definition file(s).";
      ::Error( fHere, "%s", ostr.str().c_str() );
      return -2;
    }
  }

  return 0;
}

//_____________________________________________________________________________
Int_t FormulaHandler::LinkTo( HistogramAxis& axis )
{
  return axis.Init( fFormula );
}

//_____________________________________________________________________________
Output::BranchHandler*
VariableHandlerFactory::Create( css_t& branchname,
				css_t& /* definition */ )
{
  // Create a variable branch handler and add it to the list of branches

  const THaVar* pvar = gHaVars->Find(branchname.c_str());
  if (!pvar) {
    ostringstream ostr;
    ostr << "Global variable " << branchname << " does not exist." << endl;
    ostr << "There is probably a typo error... ";
    ::Warning( fHere, "%s", ostr.str().c_str() );
    return 0;
  }
  BranchHandler* h = new VariableHandler(pvar,gHaVars,kVar);
  return h;
}

//_____________________________________________________________________________
Output::BranchHandler*
FormulaHandlerFactory::Create( css_t& branchname, css_t& formdef )
{
  // Create a formula and its branch handler

  THaFormula* pform = new THaFormula( branchname.c_str(), formdef.c_str() );
  if( !pform || pform->IsError() ) {
    ostringstream ostr;
    ostr << "Error in formula " << branchname << endl;
    ostr << "There is probably a typo error... ";
    ::Warning( fHere, "%s", ostr.str().c_str() );
    return 0;
  }
  BranchHandler* h = new FormulaHandler(pform,kForm);
  return h;
}

//_____________________________________________________________________________
Output::BranchHandler*
CutHandlerFactory::Create( css_t& branchname, css_t& cutdef )
{
  // Create a cut and its branch handler

  //FIXME: we need array cuts!
  THaFormula* pcut = new THaFormula( branchname.c_str(), cutdef.c_str() );
  if( !pcut || pcut->IsError() ) {
    ostringstream ostr;
    ostr << "Error in cut " << branchname << endl;
    ostr << "There is probably a typo error... ";
    ::Warning( fHere, "%s", ostr.str().c_str() );
    return 0;
  }
  BranchHandler* h = new FormulaHandler(pcut,kCut);
  return h;
}

//_____________________________________________________________________________
bool VariableHandlerFactory::IsEqual( css_t& branchname, css_t& definition,
			      const BranchHandler* existing_handler ) const
{
  if( existing_handler->GetType() == kVar ) {
    assert( dynamic_cast<const VariableHandler*>(existing_handler) != 0 );
    const THaVar* pvar =
      static_cast<const VariableHandler*>(existing_handler)->GetVariable();
    if( branchname == pvar->GetName() )
      return true;
  }
  return false;
}

//_____________________________________________________________________________
bool FormulaHandlerFactory::IsEqual( css_t& branchname, css_t& definition,
			      const BranchHandler* existing_handler ) const
{
  if( existing_handler->GetType() == kVar ) {
    assert( dynamic_cast<const FormulaHandler*>(existing_handler) != 0 );
    const THaFormula* pform =
      static_cast<const FormulaHandler*>(existing_handler)->GetFormula();
    if( branchname == pform->GetName() && definition == pform->GetTitle() )
      return true;
  }
  return false;
}

//_____________________________________________________________________________

} // namespace Podd
