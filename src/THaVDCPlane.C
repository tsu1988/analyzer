///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPlane                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Here:                                                                     //
//        Units of measurements:                                             //
//        For cluster position (center) and size  -  wires;                  //
//        For X, Y, and Z coordinates of track    -  meters;                 //
//        For Theta and Phi angles of track       -  in tangents.            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//#define CLUST_RAWDATA_HACK

#include "THaVDC.h"
#include "THaVDCPlane.h"
#include "THaVDCWire.h"
#include "THaVDCChamber.h"
#include "THaVDCClusterFitter.h"
#include "THaVDCHit.h"
#include "THaDetMap.h"
#include "THaVDCAnalyticTTDConv.h"
#include "THaEvData.h"
#include "TString.h"
#include "TClass.h"
#include "TMath.h"
#include "VarDef.h"
#include "THaApparatus.h"
#include "THaTriggerTime.h"

#include <vector>
#include <set>
#include <algorithm>
#include <numeric>
#include <utility>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cassert>

#ifdef CLUST_RAWDATA_HACK
#include <fstream>
#endif

using namespace std;
using namespace VDC;

#define ALL(c) (c).begin(), (c).end()

typedef vector<Vhit_t> Region_t;

//TODO: put some of these into a common header file
// Helpers
//___________________________________________________________________________
template< typename VectorElem > inline void
NthCombination( UInt_t n,
		typename vector<vector<VectorElem> >::const_iterator iv,
		typename vector<vector<VectorElem> >::const_iterator ie,
		vector<VectorElem>& selected )
{
  // Get the n-th combination of the elements in "vec" and
  // put result in "selected". selected[k] is one of the
  // vec[k].size() elements in the k-th plane.
  //
  // This function is equivalent to vec.size() nested/recursive loops
  // over the elements of each of the vectors in vec.

  assert( ie-iv > 0 );

  selected.resize( ie-iv );
  typename vector<VectorElem>::iterator is = selected.begin();
  while( iv != ie ) {
    typename vector<VectorElem>::size_type npt = (*iv).size(), k;
    assert(npt);
    if( npt == 1 )
      k = 0;
    else {
      k = n % npt;
      n /= npt;
    }
    // Copy the selected element
    (*is) = (*iv)[k];
    ++iv;
    ++is;
  }
}

//___________________________________________________________________________
template< typename Container > struct SizeMul :
  public std::binary_function< typename Container::size_type, Container,
			       typename Container::size_type >
{
  // Function object to get the product of the sizes of the containers in a
  // container, ignoring empty ones
  typedef typename Container::size_type csiz_t;
  csiz_t operator() ( csiz_t val, const Container& c ) const
  { return ( c.empty() ? val : val * c.size() ); }
};

//___________________________________________________________________________
// Cluster candidate sorting functor
struct CandidateSorter :
  public std::binary_function< THaVDCCluster, THaVDCCluster, bool >
{
  bool operator() ( const THaVDCCluster& a, const THaVDCCluster& b ) const
  {
    // Sort by pivot wire number (lowest first), then NDoF (largest first),
    // then chi2 of the fit
    if( a.GetPivotWireNum() != b.GetPivotWireNum() )
      return (a.GetPivotWireNum() < b.GetPivotWireNum());
    if( a.GetNDoF() != b.GetNDoF() )
      return (a.GetNDoF() > b.GetNDoF());
    return (a.GetChi2() < b.GetChi2());
  }
};

// Though extremely unlikely, two clusters may end up with the same ndof and
// chi2 of their fits, so this needs to be a multiset
typedef multiset<THaVDCCluster,CandidateSorter> ClustSet_t;
typedef ClustSet_t::const_iterator clust_iter_t;

//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent), /*fTable(0),*/
    fClustChi2Cut(3), fTTDConv(0), fVDC(0), fglTrg(0)
{
  // Constructor

  fWires.reserve(368);
  fHits.reserve(100);
  fClusters.reserve(20);

  fVDC = dynamic_cast<THaVDC*>( GetMainDetector() );
}

//_____________________________________________________________________________
THaVDCPlane::~THaVDCPlane()
{
  // Destructor.

  if( fIsSetup )
    RemoveVariables();
  delete fTTDConv;
//   delete [] fTable;
}

//_____________________________________________________________________________
THaVDCCluster* THaVDCPlane::AddCluster( const THaVDCCluster& newCluster )
{
  // Add given cluster to the fClusters array

  assert( GetNClusters() >= 0 );
  fClusters.push_back( newCluster );
  return &fClusters.back();
}

//_____________________________________________________________________________
void THaVDCPlane::Clear( Option_t* )
{
  // Clears the contents of the and hits and clusters
  fNHits = fNWiresHit = 0;
  fHits.clear();
  fClusters.clear();
}

//_____________________________________________________________________________
void THaVDCPlane::MakePrefix()
{
  // Special treatment of the prefix for VDC planes: We don't want variable
  // names such as "R.vdc.uv1.u.x", but rather "R.vdc.u1.x".

  TString basename;
  THaDetectorBase* uv_plane = GetParent();

  if( fVDC )
    basename = fVDC->GetPrefix();
  if( fName.Contains("u") )
    basename.Append("u");
  else if ( fName.Contains("v") )
    basename.Append("v");
  if( uv_plane && strstr( uv_plane->GetName(), "uv1" ))
    basename.Append("1.");
  else if( uv_plane && strstr( uv_plane->GetName(), "uv2" ))
    basename.Append("2.");
  if( basename.Length() == 0 )
    basename = fName + ".";

  delete [] fPrefix;
  fPrefix = new char[ basename.Length()+1 ];
  strcpy( fPrefix, basename.Data() );
}

//_____________________________________________________________________________
Int_t THaVDCPlane::ReadDatabase( const TDatime& date )
{
  // Load wire and plane parameters from database. Populate wire array.

  const char* const here = __FUNCTION__;
  const char* const action = "Fix database.";

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  const int LEN = 100;
  char buff[LEN];

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix); tag.Chop(); // delete trailing dot of prefix
  tag.Prepend("["); tag.Append("]");
  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section \"%s\" not found! "
	  "Initialization failed", tag.Data() );
    fclose(file);
    return kInitError;
  }

  //Found the entry for this plane, so read data
  fWires.clear();
  Int_t nWires = 0;    // Number of wires to create
  Int_t prev_first = 0, prev_nwires = 0;
  // Set up the detector map
  fDetMap->Clear();
  do {
    fgets( buff, LEN, file );
    // bad kludge to allow a variable number of detector map lines
    if( strchr(buff, '.') ) // any floating point number indicates end of map
      break;
    // Get crate, slot, low channel and high channel from file
    Int_t crate, slot, lo, hi;
    if( sscanf( buff, "%6d %6d %6d %6d", &crate, &slot, &lo, &hi ) != 4 ) {
      if( *buff ) buff[strlen(buff)-1] = 0; //delete trailing newline
      Error( Here(here), "Error reading detector map line: %s", buff );
      fclose(file);
      return kInitError;
    }
    Int_t first = prev_first + prev_nwires;
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi, first);
    prev_first = first;
    prev_nwires = (hi - lo + 1);
    nWires += prev_nwires;
  } while( *buff );  // sanity escape
  // Load z, wire beginning postion, wire spacing, and wire angle
  sscanf( buff, "%15lf %15lf %15lf %15lf", &fZ, &fWBeg, &fWSpac, &fWAngle );
  fWAngle *= TMath::Pi()/180.0; // Convert to radians
  fSinAngle = TMath::Sin( fWAngle );
  fCosAngle = TMath::Cos( fWAngle );

  // Load drift velocity (will be used to initialize crude Time to Distance
  // converter)
  fscanf(file, "%15lf", &fDriftVel);
  fgets(buff, LEN, file); // Read to end of line
  fgets(buff, LEN, file); // Skip line

  // first read in the time offsets for the wires
  float* wire_offsets = new float[nWires];
  int*   wire_nums    = new int[nWires];

  for (int i = 0; i < nWires; i++) {
    int wnum = 0;
    float offset = 0.0;
    fscanf(file, " %6d %15f", &wnum, &offset);
    wire_nums[i] = wnum-1; // Wire numbers in file start at 1
    wire_offsets[i] = offset;
  }


  // now read in the time-to-drift-distance lookup table
  // data (if it exists)
//   fgets(buff, LEN, file); // read to the end of line
//   fgets(buff, LEN, file);
//   if(strncmp(buff, "TTD Lookup Table", 16) == 0) {
//     // if it exists, read the data in
//     fscanf(file, "%le", &fT0);
//     fscanf(file, "%d", &fNumBins);

//     // this object is responsible for the memory management
//     // of the lookup table
//     delete [] fTable;
//     fTable = new Float_t[fNumBins];
//     for(int i=0; i<fNumBins; i++) {
//       fscanf(file, "%e", &(fTable[i]));
//     }
//   } else {
//     // if not, set some reasonable defaults and rewind the file
//     fT0 = 0.0;
//     fNumBins = 0;
//     fTable = NULL;
//     cout<<"Could not find lookup table header: "<<buff<<endl;
//     fseek(file, -strlen(buff), SEEK_CUR);
//   }

  // Define time-to-drift-distance converter
  // Currently, we use the analytic converter.
  // FIXME: Let user choose this via a parameter
  delete fTTDConv;
  fTTDConv = new THaVDCAnalyticTTDConv(fDriftVel);

  //THaVDCLookupTTDConv* ttdConv = new THaVDCLookupTTDConv(fT0, fNumBins, fTable);

  // Now initialize wires (those wires... too lazy to initialize themselves!)
  // Caution: This may not correspond at all to actual wire channels!
  for (int i = 0; i < nWires; i++) {
    fWires.push_back( THaVDCWire(i, fWBeg+i*fWSpac, wire_offsets[i], fTTDConv) );
    if( wire_nums[i] < 0 )
      fWires.back().SetFlag(1);
  }
  delete [] wire_offsets;
  delete [] wire_nums;

  // Set location of chamber center (in detector coordinates)
  fOrigin.SetXYZ( 0.0, 0.0, fZ );

  // Read additional parameters (not in original database)
  // For backward compatibility with database version 1, these parameters
  // are in an optional section, labelled [ <prefix>.extra_param ]
  // (e.g. [ R.vdc.u1.extra_param ]) or [ R.extra_param ].  If this tag is
  // not found, a warning is printed and defaults are used.

  tag = "["; tag.Append(fPrefix); tag.Append("extra_param]");
  TString tag2(tag);
  found = false;
  rewind(file);
  while (!found && fgets(buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    // Poor man's default key search
    rewind(file);
    tag2 = fPrefix;
    Ssiz_t pos = tag2.Index(".");
    if( pos != kNPOS )
      tag2 = tag2(0,pos+1);
    else
      tag2.Append(".");
    tag2.Prepend("[");
    tag2.Append("extra_param]");
    while (!found && fgets(buff, LEN, file) != NULL) {
      char* buf = ::Compress(buff);  //strip blanks
      line = buf;
      delete [] buf;
      if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
      if ( tag2 == line )
	found = true;
    }
  }
  if( found ) {
    if( fscanf(file, "%lf %lf", &fTDCRes, &fT0Resolution) != 2) {
      Error( Here(here), "Error reading TDC scale and T0 resolution\n"
	     "Line = %s\n%s", buff, action );
      fclose(file);
      return kInitError;
    }
    fgets(buff, LEN, file); // Read to end of line
    if( fscanf( file, "%d %d %d %d %d %lf", &fMinClustSize, &fMaxClustSpan,
		&fNMaxGap, &fMinTime, &fMaxTime, &fClustChi2Cut ) != 6 ) {
      Error( Here(here), "Error reading min_clust_size, max_clust_span, "
	     "max_gap, min_time or max_time, chi2_cut.\nLine = %s\n%s",
	     buff, action );
      fclose(file);
      return kInitError;
    }
    fgets(buff, LEN, file); // Read to end of line
    // Time-to-distance converter parameters
    if( THaVDCAnalyticTTDConv* analytic_conv =
	dynamic_cast<THaVDCAnalyticTTDConv*>(fTTDConv) ) {
      // THaVDCAnalyticTTDConv
      // Format: 4*A1 4*A2 dtime(s)  (9 floats)
      Double_t A1[4], A2[4], dtime;
      if( fscanf(file, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
		 &A1[0], &A1[1], &A1[2], &A1[3],
		 &A2[0], &A2[1], &A2[2], &A2[3], &dtime ) != 9) {
	Error( Here(here), "Error reading THaVDCAnalyticTTDConv parameters\n"
	       "Line = %s\nExpect 9 floating point numbers. %s",
	       buff, action );
	fclose(file);
	return kInitError;
      } else {
	analytic_conv->SetParameters( A1, A2, dtime );
      }
    }
//     else if( (... *conv = dynamic_cast<...*>(fTTDConv)) ) {
//       // handle parameters of other TTD converters here
//     }

    fgets(buff, LEN, file); // Read to end of line

    Double_t h, w;

    if( fscanf(file, "%lf %lf", &h, &w) != 2) {
	Error( Here(here), "Error reading height/width parameters\nLine = %s\n"
	       "Expecting 2 floating point numbers. %s", buff, action );
	fclose(file);
	return kInitError;
      } else {
	   fSize[0] = h/2.0;
	   fSize[1] = w/2.0;
      }

    fgets(buff, LEN, file); // Read to end of line
    // Sanity checks
    if( fMinClustSize < 1 || fMinClustSize > 6 ) {
      Error( Here(here), "Invalid min_clust_size = %d, must be between "
	     "1 and 6. %s", fMinClustSize, action );
      fclose(file);
      return kInitError;
    }
    if( fMaxClustSpan < 2 || fMaxClustSpan > 12 ) {
      Error( Here(here), "Invalid max_clust_span = %d, must be between "
	     "1 and 12. %s", fMaxClustSpan, action );
      fclose(file);
      return kInitError;
    }
    if( fNMaxGap < 0 || fNMaxGap > 2 ) {
      Error( Here(here), "Invalid max_gap = %d, must be between "
	     "0 and 2. %s", fNMaxGap, action );
      fclose(file);
      return kInitError;
    }
    if( fMinTime < 0 || fMinTime > 4095 ) {
      Error( Here(here), "Invalid min_time = %d, must be between "
	     "0 and 4095. %s", fMinTime, action );
      fclose(file);
      return kInitError;
    }
    if( fMaxTime < 1 || fMaxTime > 4096 || fMinTime >= fMaxTime ) {
      Error( Here(here), "Invalid max_time = %d. Must be between 1 and 4096 "
	     "and >= min_time = %d. %s", fMaxTime, fMinTime, action );
      fclose(file);
      return kInitError;
    }
    if( fClustChi2Cut == 0 ) {
      Warning( Here(here), "Zero chi2_cut makes no sense and will give no "
	       "tracks. %s", action );
    }
    if( fClustChi2Cut < 0 ) {
      Warning( Here(here), "Negative chi2_cut = %lf changed to positive. %s",
	       fClustChi2Cut, action );
      fClustChi2Cut = TMath::Abs(fClustChi2Cut);
    }
  } else {
    Warning( Here(here), "No database section \"%s\" or \"%s\" found. "
	     "Using defaults.", tag.Data(), tag2.Data() );
    fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan
    fT0Resolution = 6e-8; // 60 ns --- crude guess
    fMinClustSize = 4;
    fMaxClustSpan = 7;
    fNMaxGap = 0;
    fMinTime = 800;
    fMaxTime = 2200;
    fClustChi2Cut = 3.;

    if( THaVDCAnalyticTTDConv* analytic_conv =
	dynamic_cast<THaVDCAnalyticTTDConv*>(fTTDConv) ) {
      Double_t A1[4], A2[4], dtime = 4e-9;
      A1[0] = 2.12e-3;
      A1[1] = A1[2] = A1[3] = 0.0;
      A2[0] = -4.2e-4;
      A2[1] = 1.3e-3;
      A2[2] = 1.06e-4;
      A2[3] = 0.0;
      analytic_conv->SetParameters( A1, A2, dtime );
    }
  }

  THaDetectorBase *sdet = GetParent();
  if( sdet )
    fOrigin += sdet->GetOrigin();

  // finally, find the timing-offset to apply on an event-by-event basis
  //FIXME: time offset handling should go into the enclosing apparatus -
  //since not doing so leads to exactly this kind of mess:
  THaApparatus* app = GetApparatus();
  const char* nm = "trg"; // inside an apparatus, the apparatus name is assumed
  if( !app ||
      !(fglTrg = dynamic_cast<THaTriggerTime*>(app->GetDetector(nm))) ) {
    Warning(Here(here),"Trigger-time detector \"%s\" not found. "
	    "Event-by-event time offsets will NOT be used!!",nm);
  }

  fIsInit = true;
  fclose(file);

  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::DefineVariables( EMode mode )
{
  // initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list

  RVarDef vars[] = {
    { "nhit",   "Number of hits",             "GetNHits()" },
    { "wire",   "Active wire numbers",        "fHits.THaVDCHit.GetWireNum()" },
    { "rawtime","Raw TDC values of wires",    "fHits.THaVDCHit.fRawTime" },
    { "time",   "TDC values of active wires", "fHits.THaVDCHit.fTime" },
    { "dist",   "Drift distances",            "fHits.THaVDCHit.fDist" },
    { "ddist",  "Drft dist uncertainty",      "fHits.THaVDCHit.fdDist" },
    { "trdist", "Dist. from track",           "fHits.THaVDCHit.ftrDist" },
    { "ltrdist","Dist. from local track",     "fHits.THaVDCHit.fltrDist" },
    { "trknum", "Track number (0=unused)",    "fHits.THaVDCHit.fTrkNum" },
    { "clsnum", "Cluster number (-1=unused)", "fHits.THaVDCHit.fClsNum" },
    { "nclust", "Number of clusters",         "GetNClusters()" },
    { "clsiz",  "Cluster sizes",              "fClusters.THaVDCCluster.fSize" },
    { "clpivot","Cluster pivot wire num",     "fClusters.THaVDCCluster.GetPivotWireNum()" },
    { "clpos",  "Cluster intercepts (m)",     "fClusters.THaVDCCluster.fInt" },
    { "slope",  "Cluster best slope",         "fClusters.THaVDCCluster.fSlope" },
    { "lslope", "Cluster local (fitted) slope","fClusters.THaVDCCluster.fLocalSlope" },
    { "t0",     "Timing offset (s)",          "fClusters.THaVDCCluster.fT0" },
    { "sigsl",  "Cluster slope error",        "fClusters.THaVDCCluster.fSigmaSlope" },
    { "sigpos", "Cluster position error (m)", "fClusters.THaVDCCluster.fSigmaInt" },
    { "sigt0",  "Timing offset error (s)",    "fClusters.THaVDCCluster.fSigmaT0" },
    { "clchi2", "Cluster chi2",               "fClusters.THaVDCCluster.fChi2" },
    { "clndof", "Cluster NDoF",               "fClusters.THaVDCCluster.fNDoF" },
    { "cltcor", "Cluster Time correction",    "fClusters.THaVDCCluster.fTimeCorrection" },
    { "cltridx","Idx of track assoc w/cluster", "fClusters.THaVDCCluster.GetTrackIndex()" },
    { "cltrknum", "Cluster track number (0=unused)", "fClusters.THaVDCCluster.fTrkNum" },
    { "clbeg", "Cluster start wire",          "fClusters.THaVDCCluster.fClsBeg" },
    { "clend", "Cluster end wire",            "fClusters.THaVDCCluster.fClsEnd" },
    { "npass", "Number of hit passes for cluster", "GetNpass()" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );

}

//_____________________________________________________________________________
Int_t THaVDCPlane::Decode( const THaEvData& evData )
{
  // Converts the raw data into hit information
  // Logical wire numbers a defined by the detector map. Wire number 0
  // corresponds to the first defined channel, etc.

  // TODO: Support "reversed" detector map modules a la MWDC

  if (!evData.IsPhysicsTrigger()) return -1;

  // the event's T0-shift, due to the trigger-type
  // only an issue when adding in un-retimed trigger types
  Double_t evtT0=0;
  if ( fglTrg && fglTrg->Decode(evData)==kOK ) evtT0 = fglTrg->TimeOffset();

  Int_t nextHit = 0;

  bool only_fastest_hit = false, no_negative = false;
  if( fVDC ) {
    // If true, add only the first (earliest) hit for each wire
    only_fastest_hit = fVDC->TestBit(THaVDC::kOnlyFastest);
    // If true, ignore negative drift times completely
    no_negative      = fVDC->TestBit(THaVDC::kIgnoreNegDrift);
  }

  // Loop over all detector modules for this wire plane
  for (Int_t i = 0; i < fDetMap->GetSize(); i++) {
    THaDetMap::Module * d = fDetMap->GetModule(i);

    // Get number of channels with hits
    Int_t nChan = evData.GetNumChan(d->crate, d->slot);
    for (Int_t chNdx = 0; chNdx < nChan; chNdx++) {
      // Use channel index to loop through channels that have hits

      Int_t chan = evData.GetNextChan(d->crate, d->slot, chNdx);
      if (chan < d->lo || chan > d->hi)
	continue; //Not part of this detector

      // Wire numbers and channels go in the same order ...
      Int_t wireNum  = d->first + chan - d->lo;
      THaVDCWire* wire = GetWire(wireNum);
      if( !wire || wire->GetFlag() != 0 ) continue;

      // Get number of hits for this channel and loop through hits
      Int_t nHits = evData.GetNumHits(d->crate, d->slot, chan);

      Int_t max_data = -1;
      Double_t toff = wire->GetTOffset();

      for (Int_t hit = 0; hit < nHits; hit++) {

	// Now get the TDC data for this hit
	Int_t data = evData.GetData(d->crate, d->slot, chan, hit);

	// Convert the TDC value to the drift time.
	// Being perfectionist, we apply a 1/2 channel correction to the raw
	// TDC data to compensate for the fact that the TDC truncates, not
	// rounds, the data.
	Double_t xdata = static_cast<Double_t>(data) + 0.5;
	Double_t time = fTDCRes * (toff - xdata) - evtT0;

	// If requested, ignore hits with negative drift times
	// (due to noise or miscalibration). Use with care.
	// If only fastest hit requested, find maximum TDC value and record the
	// hit after the hit loop is done (see below).
	// Otherwise just record all hits.
	if( !no_negative || time > 0.0 ) {
	  if( only_fastest_hit ) {
	    if( data > max_data )
	      max_data = data;
	  } else {
	    fHits.push_back( THaVDCHit(wire, data, time) );
	    nextHit++;
	  }
	}

    // Count all hits and wires with hits
    //    fNWiresHit++;

      } // End hit loop

      // If we are only interested in the hit with the largest TDC value
      // (shortest drift time), it is recorded here.
      if( only_fastest_hit && max_data>0 ) {
	Double_t xdata = static_cast<Double_t>(max_data) + 0.5;
	Double_t time = fTDCRes * (toff - xdata) - evtT0;
	fHits.push_back( THaVDCHit(wire, max_data, time) );
	nextHit++;
      }
    } // End channel index loop
  } // End slot loop

  // Sort the hits in order of increasing wire position and, for the same wire,
  // by increasing drift time (see THaVDCHit::operator<)
  sort( ALL(fHits) );

#ifdef WITH_DEBUG
  if ( fDebug > 3 ) {
    printf("\nVDC %s:\n",GetPrefix());
    int ncol=4;
    for (int i=0; i<ncol; i++) {
      printf("     Wire    TDC  ");
    }
    printf("\n");

    for (int i=0; i<(nextHit+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*nextHit/ncol+i;
	if (ind < nextHit) {
	  THaVDCHit* hit = GetHit(ind);
	  printf("     %3d    %5d ",hit->GetWireNum(),hit->GetRawTime());
	} else {
	  //	  printf("\n");
	  break;
	}
      }
      printf("\n");
    }
  }
#endif

  return 0;

}


//_____________________________________________________________________________
class TimeCut {
public:
  TimeCut( THaVDC* vdc, THaVDCPlane* _plane )
    : hard_cut(false), soft_cut(false), maxdist(0.0), plane(_plane)
  {
    assert(vdc);
    assert(plane);
    if( vdc ) {
      hard_cut = vdc->TestBit(THaVDC::kHardTDCcut);
      soft_cut = vdc->TestBit(THaVDC::kSoftTDCcut);
    }
    if( soft_cut ) {
      maxdist =
	0.5*static_cast<THaVDCChamber*>(plane->GetParent())->GetSpacing();
      if( maxdist == 0.0 )
	soft_cut = false;
    }
  }
  bool operator() ( const THaVDCHit* hit )
  {
    // Only keep hits whose drift times are within sanity cuts
    if( hard_cut ) {
      Double_t rawtime = hit->GetRawTime();
      if( rawtime < plane->GetMinTime() || rawtime > plane->GetMaxTime())
	return false;
    }
    if( soft_cut ) {
      Double_t ratio = hit->GetTime() * plane->GetDriftVel() / maxdist;
      if( ratio < -0.5 || ratio > 1.5 )
	return false;
    }
    return true;
  }
private:
  bool          hard_cut;
  bool          soft_cut;
  double        maxdist;
  THaVDCPlane*  plane;
};

//_____________________________________________________________________________
static inline UInt_t Span( Region_t::const_iterator start,
			   Region_t::const_iterator end )
{
  assert( start < end );
  Int_t diff =
    (*(end-1)).front()->GetWireNum() - (*start).front()->GetWireNum();
  assert( diff >= 0 );
  return diff;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::FindClusters()
{
  // Find clusters of hits in a VDC plane.

  assert( GetNClusters() == 0 );   // Clear() already called

  TimeCut timecut(fVDC,this);

  Int_t nHits = GetNHits();   // Number of hits in the plane
  for( Int_t i = 0; i < nHits; ) {
    THaVDCHit* hit = GetHit(i);
    assert(hit);
    if( !hit or !timecut(hit) ) {
      ++i;
      continue;
    }
    // First hit of a new region
    Region_t region( 1, Vhit_t(1,hit) ); // All hits in a region of interest

    // Add hits to region
    Vhit_t::size_type nwires = 1;
    Bool_t multi_hit = false;
    while( ++i < nHits ) {
      THaVDCHit* nextHit = GetHit(i);
      assert( nextHit );    // else bug in Decode
      if( !nextHit || !timecut(nextHit) )
	continue;
      Int_t ndif = nextHit->GetWireNum() - hit->GetWireNum();
      if( ndif != 0 && IsReversed() ) ndif = -ndif;
      assert( ndif >= 0 );  // else hits not sorted correctly
      // A gap of more than fNMaxGap wires unambiguously separates
      // regions of interest
      if( static_cast<UInt_t>(ndif) > fNMaxGap+1 )
	break;
      // Add hit to appropriate array of hits on given wire number
      if( ndif == 0 ) {
	assert( !(region.empty() || region.back().empty()) );
	assert( nextHit->GetWireNum() == region.back().back()->GetWireNum() );
	region.back().push_back(nextHit);
	multi_hit = true;
      } else {
	region.push_back( Vhit_t(1,nextHit) );
	++nwires;    // unique wires
      }
      hit = nextHit;
    }
    assert( i <= nHits );
    assert( nwires == region.size() );

    if( nwires < fMinClustSize )
      continue;

    // We have a region of interest
    ClustSet_t candidates;
    UInt_t region_span = Span(ALL(region));
    Bool_t simple_case = !multi_hit && (region_span <= fMaxClustSpan);
    Bool_t multi_pivot = false;
    Bool_t go = true;
    THaVDCHit* prev_pivot = 0;
    for( UInt_t slice_size = min(fMaxClustSpan,region_span)+1;
	 slice_size >= fMinClustSize && go; --slice_size ) {
      for( Region_t::const_iterator start = region.begin(),
	     end = start+slice_size; end <= region.end() && go;
	   ++start, ++end ) {

	UInt_t ncombos = 1;
	if( multi_hit ) {
	  try {
	    ncombos = accumulate( start, end, (UInt_t)1, SizeMul<Vhit_t>() );
	  }
	  catch( overflow_error ) {
	    continue;
	  }
	}
#ifndef NDEBUG
	else {
	  ncombos = accumulate( start, end, (UInt_t)1, SizeMul<Vhit_t>() );
	  assert( ncombos == 1 );
	}
#endif
	ClusterFitter clust( this, slice_size );
	clust.SetMaxT0( 1.5*fT0Resolution );
	for( UInt_t i = 0; i < ncombos; ++i ) {
	  clust.Clear();
	  NthCombination( i, start, end, clust.GetHits() );
	  assert( clust.GetSize() == static_cast<Int_t>(slice_size) );

	  if( !clust.EstTrackParameters() )
	    continue;
	  clust.ConvertTimeToDist( clust.GetT0() );
	  clust.FitTrack( ClusterFitter::kLinearT0 );
	  if( !clust.IsFitOK() ||
	      TMath::Abs(clust.GetT0()) > fT0Resolution )
	    continue;
	  assert( clust.GetPivot() != clust.GetHits().front() &&
		  clust.GetPivot() != clust.GetHits().back() );

	  // clust.ClearFit();
	  // clust.ConvertTimeToDist( clust.GetT0() );
	  // clust.FitTrack( ClusterFitter::kT0 );
	  // if( !clust.IsFitOK() )
	  //   continue;

	  // Reject apparently bad fits. A hard cut like this always eats
	  // into the tracking efficiency. The better the time-to-distance
	  // conversion, the more efficient this cut will be, so careful
	  // calibrations are important
	  assert( clust.GetNDoF() > 0 );
	  if( clust.GetChi2()/clust.GetNDoF() > fClustChi2Cut )
	    continue;

	  clust.SetBegEnd();

	  if( prev_pivot && prev_pivot != clust.GetPivot() )
	    multi_pivot = true;
	  prev_pivot = clust.GetPivot();

	  // Save/sort cluster candidates in a set. Note that the insertion will
	  // copy-construct a THaVDCCluster from the ClusterFitter.
	  // The set will contain clusters that are sorted by CandidateSorter.
	  clust_iter_t ins = candidates.insert( clust );
	  (*ins).ClaimHits();

	  // Accelerate things for the simple case: no multihits (-> one pivot),
	  // a region small enough so that there is only one top-level slice,
	  // and a successful fit over that top-level slice (=full region)
	  if( simple_case && slice_size == region_span+1 ) {
	    go = false;
	    break;
	  }
	}
      }
    }
#ifdef WITH_DEBUG
    if( fDebug > 2 )
      for_each( ALL(candidates), bind2nd(mem_fun_ref(&THaVDCCluster::Print),"") );
#endif
    // For each given pivot wire, eliminate candidates whose hits are merely
    // subsets of another candidate's hits.
    for( clust_iter_t it = candidates.begin(); it != candidates.end(); ) {
      const THaVDCCluster& topCluster = *it;
      AddCluster( topCluster );
      THaVDCHit* curPivot = topCluster.GetPivot();
      Bool_t shared_hits = multi_pivot && topCluster.HasSharedHits();
      while( ++it != candidates.end() && (*it).GetPivot() == curPivot ) {
	const THaVDCCluster& nextCluster = *it;
	assert( nextCluster.GetSize() <= topCluster.GetSize() );
	//TODO: assert cluster hit vectors are ordered
	if( nextCluster.GetSize() == topCluster.GetSize() ) {
	  // Same-size clusters. nextCluster has larger chi2 than topCluster.
	  // We keep it nevertheless because the global slope from combining
	  // it with other clusters may give a better overall chi2.
	  assert( !includes( ALL(topCluster.GetHits()), ALL(nextCluster.GetHits()),
			     THaVDCHit::ByPosThenTime() ));
	  assert( nextCluster.GetNDoF() == topCluster.GetNDoF() );
	  assert( nextCluster.GetChi2() > topCluster.GetChi2() );
	  AddCluster( nextCluster );
	}
	else if( !includes( ALL(topCluster.GetHits()), ALL(nextCluster.GetHits()),
			    THaVDCHit::ByPosThenTime() )) {
	  AddCluster( nextCluster );
	}
	else if( shared_hits ) {
	  AddCluster( nextCluster );
	  if( !nextCluster.HasSharedHits() )
	    shared_hits = false;
	}
      }
    }
    //TODO:
    // - fix cluster fit routines
    // - implement hit sharing detection in THaVDC::ConstructTracks
  }

  return GetNClusters();  // return the number of clusters found
}

//_____________________________________________________________________________
Int_t THaVDCPlane::FitTracks()
{
  // Fit tracks to cluster positions and drift distances.

  Int_t nClust = GetNClusters();
  for (int i = 0; i < nClust; i++) {
    THaVDCCluster* clust = GetCluster(i);
    if( !clust ) continue;

    // Convert drift times to distances.
    // The conversion algorithm is determined at wire initialization time,
    // i.e. currently in the ReadDatabase() function of this class.
    // The conversion is done with the current value of fSlope in the
    // clusters, i.e. either the rough guess from
    // THaVDCCluster::EstTrackParameters or the global slope from
    // THaVDC::ConstructTracks

    //FIXME: this is now done when clusters are built
    // clust->ConvertTimeToDist();

    // // Fit drift distances to get intercept, slope.
    // clust->FitTrack();

#ifdef CLUST_RAWDATA_HACK
    // HACK: write out cluster info for small-t0 clusters in u1
    if( fName == "u" && !strcmp(GetParent()->GetName(),"uv1") &&
	TMath::Abs(clust->GetT0()) < fT0Resolution/3. &&
	clust->GetSize() <= 6 ) {
      ofstream outp;
      outp.open("u1_cluster_data.out",ios_base::app);
      outp << clust->GetSize() << endl;
      for( int i=clust->GetSize()-1; i>=0; i-- ) {
	outp << clust->GetHit(i)->GetPos() << " "
	     << clust->GetHit(i)->GetDist()
	     << endl;
      }
      outp << 1./clust->GetSlope() << " "
	   << clust->GetIntercept()
	   << endl;
      outp.close();
    }
#endif
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCPlane)
