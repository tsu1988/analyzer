//*-- Author :    Ole Hansen   16/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaDetMap
//
// The standard detector map for a Hall A detector.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetMap.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

const int THaDetMap::kDetMapSize;

//_____________________________________________________________________________
THaDetMap::THaDetMap() : fNmodules(0)
{
  // Default constructor. Create an empty detector map.

  fMaplength = 10;      // default number of modules
  fMap = new Module [fMaplength];

}

//_____________________________________________________________________________
THaDetMap::THaDetMap( const THaDetMap& rhs )
{
  // Copy constructor. Initialize one detector map with another.

  fMaplength = rhs.fMaplength;
  fMap = new Module [fMaplength];

  fNmodules = rhs.fNmodules;

  memcpy(fMap,rhs.fMap,fNmodules*sizeof(Module));
}

//_____________________________________________________________________________
THaDetMap& THaDetMap::operator=( const THaDetMap& rhs )
{
  // THaDetMap assignment operator. Assign one map to another.

  if ( this != &rhs ) {
    fNmodules = rhs.fNmodules;
    if ( fMaplength != rhs.fMaplength ) {
      delete [] fMap;
      fMaplength = rhs.fMaplength;
      fMap = new Module[fMaplength];
    }
    memcpy(fMap,rhs.fMap,fNmodules*sizeof(Module));
  }
  return *this;
}

//_____________________________________________________________________________
THaDetMap::~THaDetMap()
{
  // Destructor. Free all memory used by the detector map.

  delete [] fMap;
}

//_____________________________________________________________________________
Int_t THaDetMap::AddModule( UShort_t crate, UShort_t slot, 
			    UShort_t chan_lo, UShort_t chan_hi, UInt_t firstw,
			    UInt_t model)
{
  // Add a module to the map.

  if( fNmodules >= kDetMapSize ) return -1;  //Map is full -- sanity check
  
  if( fNmodules >= fMaplength ) {            // need to expand the Map
    Int_t oldlen = fMaplength;
    fMaplength += 10;
    Module* tmpmap = new Module[fMaplength];  // expand in groups of 10 modules
  
    memcpy(tmpmap,fMap,oldlen*sizeof(Module));
    delete [] fMap;
    fMap = tmpmap;
  }
  
  Module& m = fMap[fNmodules];
  m.crate = crate;
  m.slot = slot;
  m.lo = chan_lo;
  m.hi = chan_hi;
  m.firstw = firstw;
  m.model = model;

  return ++fNmodules;
}

//_____________________________________________________________________________
void THaDetMap::Print( Option_t* opt ) const
{
  // Print the contents of the map

  cout << "Size: " << fNmodules << endl;
  for( UShort_t i=0; i<fNmodules; i++ ) {
    Module* m = GetModule(i);
    cout << " " 
	 << setw(5) << m->crate
	 << setw(5) << m->slot 
	 << setw(5) << m->lo 
	 << setw(5) << m->hi 
	 << endl;
  }
}

// small structure to hold our set of card-model, adc-type, tdc-type entries
// Let's keep this local to this file
struct ModuleType {
  UInt_t model;
  int adc;
  int tdc;
};


static const ModuleType module_list[] =
{
  { 1875, 0, 1 },
  { 1877, 0, 1 },
  { 1881, 1, 0 },
  { 3123, 1, 0 },
  { 1182, 1, 0 },
  { 0 }
};

Bool_t THaDetMap::IsADC(Module* d) {
  const ModuleType* md = module_list;
  while ( md->model && d->model != md->model ) md++;
  return md->adc;
}

Bool_t THaDetMap::IsTDC(Module* d) {
  const ModuleType* md = module_list;
  while ( md->model && d->model != md->model ) md++;
  return md->tdc;
}
//_____________________________________________________________________________
ClassImp(THaDetMap)

