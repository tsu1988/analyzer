///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
// High precision wire chamber class.                                        //
//                                                                           //
// Units used:                                                               //
//        For X, Y, and Z coordinates of track    -  meters                  //
//        For Theta and Phi angles of track       -  tan(angle)              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaTrack.h"
#include "THaVDCUVPlane.h"
#include "THaVDCUVTrack.h"
#include "THaVDCCluster.h"
#include "THaVDCTrackID.h"
#include "THaVDCTrackPair.h"
#include "THaVDCHit.h"
#include "THaScintillator.h"
#include "THaSpectrometer.h"
#include "TMath.h"
#include "TClonesArray.h"
#include "TList.h"
#include "VarDef.h"
//#include <algorithm>
#include "TROOT.h"
#include "TBRIK.h"
#include "TRotMatrix.h"
#include "TGeometry.h"
#include "TNode.h"
#include "TSPHE.h"

#ifdef WITH_DEBUG
#include <iostream>
#endif

#define PI 3.14159

Int_t gNum = 0; // Temp. global rot matrix counter

using namespace std;

//_____________________________________________________________________________
THaVDC::THaVDC( const char* name, const char* description,
		THaApparatus* apparatus ) :
  THaTrackingDetector(name,description,apparatus), fS1(0), fS2(0), fNtracks(0) 
{
  // Constructor

  // Create Upper and Lower UV planes
  fLower   = new THaVDCUVPlane( "uv1", "Lower UV Plane", this );
  fUpper   = new THaVDCUVPlane( "uv2", "Upper UV Plane", this );
  if( !fLower || !fUpper || fLower->IsZombie() || fUpper->IsZombie() ) {
    Error( Here("THaVDC()"), "Failed to create subdetectors." );
    MakeZombie();
  }

  fUVpairs = new TClonesArray( "THaVDCTrackPair", 20 );

  // Default behavior for now
  SetBit( kOnlyFastest | kHardTDCcut );

}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaVDC::Init( const TDatime& date )
{
  // Initialize VDC. Calls standard Init(), then initializes subdetectors.

  if( IsZombie() || !fUpper || !fLower )
    return fStatus = kInitError;

  EStatus status;
  if( (status = THaTrackingDetector::Init( date )) ||
      (status = fLower->Init( date )) ||
      (status = fUpper->Init( date )) )
    return fStatus = status;

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaVDC::ReadDatabase( const TDatime& date )
{
  // Read VDC database

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // load global VDC parameters
  static const char* const here = "ReadDatabase()";
  const int LEN = 256;
  char buff[LEN];
  
  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString apparatus_prefix(fPrefix);
  Ssiz_t pos = apparatus_prefix.Index("."); 
  if( pos != kNPOS )
    apparatus_prefix = apparatus_prefix(0,pos+1);
  else
    apparatus_prefix.Append(".");
  TString tag = apparatus_prefix + "global"; 

  if( SeekDBconfig(file,tag,"") == 0 ) {
    Error(Here(here), "Database entry %s not found!", tag.Data() );
    fclose(file);
    return kInitError;
  }
  // We found the entry, now read the data

  // read in some basic constants first
  fscanf(file, "%lf", &fSpacing);
  fgets(buff, LEN, file); // Skip rest of line
  fgets(buff, LEN, file); // Skip line

  // Read in the focal plane transfer elements
  // NOTE: the matrix elements should be stored such that
  // the 3 focal plane matrix elements come first, followed by
  // the other backwards elements, starting with D000
  THaMatrixElement ME;
  char w;
  bool good = false;

  fTMatrixElems.clear();
  fDMatrixElems.clear();
  fPMatrixElems.clear();
  fYMatrixElems.clear();
  fFPMatrixElems.clear();

  // read in t000 and verify it
  ME.iszero = true;  ME.order = 0;
  // Read matrix element signature
  if( fscanf(file, "%c %d %d %d", &w, &ME.pw[0], &ME.pw[1], &ME.pw[2]) == 4) {
    if( w == 't' && ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
      good = true;
      for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
	if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	  good = false;
	  break;
	}
	if(ME.poly[p_cnt] != 0.0) {
	  ME.iszero = false;
	  ME.order = p_cnt+1;
	}
      }
    }
  }
  if( good ) 
    fFPMatrixElems.push_back(ME);
  else {
    Error(Here(here), "Could not read in Matrix Element T000!");
    fclose(file);
    return kInitError;
  }
  fscanf(file, "%*c");

  // read in y000 and verify it
  ME.iszero = true;  ME.order = 0; good = false;
  // Read matrix element signature
  if( fscanf(file, "%c %d %d %d", &w, &ME.pw[0], &ME.pw[1], &ME.pw[2]) == 4) {
    if( w == 'y' && ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
      good = true;
      for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
	if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	  good = false;
	  break;
	}
	if(ME.poly[p_cnt] != 0.0) {
	  ME.iszero = false;
	  ME.order = p_cnt+1;
	}
      }
    }
  }
  if( good )
    fFPMatrixElems.push_back(ME);
  else {
    Error(Here(here), "Could not read in Matrix Element Y000!");
    fclose(file);
    return kInitError;
  }
  fscanf(file, "%*c");

  // read in p000 and verify it
  ME.iszero = true;  ME.order = 0; good = false;
  if( fscanf(file, "%c %d %d %d", &w, &ME.pw[0], &ME.pw[1], &ME.pw[2]) == 4) {
    if( w == 'p' && ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
      good = true;
      for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
	if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	  good = false;
	  break;
	}
	if(ME.poly[p_cnt] != 0.0) {
	  ME.iszero = false;
	  ME.order = p_cnt+1;
	}
      }
    }
  }
  if( good )
    fFPMatrixElems.push_back(ME);
  else {
    Error(Here(here), "Could not read in Matrix Element P000!");
    fclose(file);
    return kInitError;
  }
  fscanf(file, "%*c");

  // Read in as many of the matrix elements as there are
  while(fscanf(file, "%c %d %d %d", &w, &ME.pw[0], 
	       &ME.pw[1], &ME.pw[2]) == 4) {

    ME.iszero = true;  ME.order = 0;
    for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
      if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	Error(Here(here), "Could not read in Matrix Element %c%d%d%d!",
	      w, ME.pw[0], ME.pw[1], ME.pw[2]);
	fclose(file);
	return kInitError;
      }

      if(ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    
    // Add this matrix element to the appropriate array
    switch(w) {
    case 'D': fDMatrixElems.push_back(ME); break;
    case 'T': fTMatrixElems.push_back(ME); break;
    case 'Y': fYMatrixElems.push_back(ME); break;
    case 'P': fPMatrixElems.push_back(ME); break;
    default:
      Error(Here(here), "Invalid Matrix Element specifier: %c!", w);
      break;
    }

    fscanf(file, "%*c");

    if(feof(file) || ferror(file))
      break;
  }

  // Compute the VDC tilt angle. It is defined with respect to the 
  // reference TRANSPORT system, so it is positive for the HRS VDCs.
  const Float_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = -fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  DefineAxes(fVDCAngle);

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // Fine geometry section in the file
  tag = apparatus_prefix + "geom";
  rewind(file);
  if( SeekDBconfig(file,tag,"") == 0 ) {
    Error(Here(here), "No VDC geometry database entry %s found", tag.Data() );
    fclose(file);
    return kInitError;
  }
  // Read geometry data
  Double_t x,y,z;
  fscanf(file, "%lf %lf %lf", &x, &y, &z);
  fgets(buff, LEN, file);
  fOrigin.SetXYZ(x,y,z);
  fscanf(file, "%f %f %f", fSize, fSize+1, fSize+2 );
  fgets(buff, LEN, file);

  // get scintillator planes for later use
  fS1 = fApparatus->GetDetector("s1");
  fS2 = fApparatus->GetDetector("s2");

  fIsInit = true;
  fclose(file);
  return kOK;
}

//_____________________________________________________________________________
THaVDC::~THaVDC()
{
  // Destructor. Delete subdetectors and remove variables from global list.

  if( fIsSetup )
    RemoveVariables();

  delete fLower;
  delete fUpper;
  delete fUVpairs;
}

//_____________________________________________________________________________
Int_t THaVDC::ConstructTracks( TClonesArray* tracks, Int_t mode )
{
  // Construct tracks from pairs of upper and lower UV tracks and add 
  // them to 'tracks'

#ifdef WITH_DEBUG
  if( fDebug>0 ) {
    cout << "-----------------------------------------------\n";
    cout << "ConstructTracks: ";
    if( mode == 0 )
      cout << "iterating";
    if( mode == 1 )
      cout << "coarse tracking";
    if( mode == 2 )
      cout << "fine tracking";
    cout << endl;
  }
#endif
  UInt_t theStage = ( mode == 1 ) ? kCoarse : kFine;

  fUVpairs->Clear();

  Int_t nUpperTracks = fUpper->GetNUVTracks();
  Int_t nLowerTracks = fLower->GetNUVTracks();

#ifdef WITH_DEBUG
  if( fDebug>0 )
    cout << "nUpper/nLower = " << nUpperTracks << "  " << nLowerTracks << endl;
#endif

  // No tracks at all -> can't have any tracks
  if( nUpperTracks == 0 && nLowerTracks == 0 ) {
#ifdef WITH_DEBUG
    if( fDebug>0 )
      cout << "No tracks.\n";
#endif
    return 0;
  }

  Int_t nTracks = 0;  // Number of valid particle tracks through the detector
  Int_t nPairs  = 0;  // Number of UV track pairs to consider

  // One plane has no tracks, the other does 
  // -> maybe recoverable with loss of precision
  // FIXME: Only do this if missing cluster recovery flag set
  if( nUpperTracks == 0 || nLowerTracks == 0 ) {
    //FIXME: Put missing cluster recovery code here
    //For now, do nothing
#ifdef WITH_DEBUG
    if( fDebug>0 ) 
      cout << "missing cluster " << nUpperTracks << " " << nUpperTracks << endl;
#endif
    return 0;
  }

  THaVDCUVTrack *track, *partner;
  THaVDCTrackPair *thePair;

  for( int i = 0; i < nLowerTracks; i++ ) {
    track = fLower->GetUVTrack(i);
    if( !track ) 
      continue;

    for( int j = 0; j < nUpperTracks; j++ ) {
      partner = fUpper->GetUVTrack(j);
      if( !partner ) 
	continue;

      // Create new UV track pair.
      thePair = new( (*fUVpairs)[nPairs++] ) THaVDCTrackPair( track, partner );

      // Explicitly mark these UV tracks as unpartnered
      track->SetPartner( NULL );
      partner->SetPartner( NULL );

      // Compute goodness of match parameter
      thePair->Analyze( fSpacing );
    }
  }
      
#ifdef WITH_DEBUG
  if( fDebug>0 )
    cout << nPairs << " pairs.\n";
#endif

  // Initialize some counters
  int n_exist, n_mod = 0;
  int n_oops=0;
  // How many tracks already exist in the global track array?
  if( tracks )
    n_exist = tracks->GetLast()+1;

  // Sort pairs in order of ascending goodness of match
  if( nPairs > 1 )
    fUVpairs->Sort();

  // Mark pairs as partners, starting with the best matches,
  // until all tracks are marked.
  for( int i = 0; i < nPairs; i++ ) {
    if( !(thePair = static_cast<THaVDCTrackPair*>( fUVpairs->At(i) )) )
      continue;

#ifdef WITH_DEBUG
    if( fDebug>0 ) {
      cout << "Pair " << i << ":  " 
	   << thePair->GetUpper()->GetUCluster()->GetPivotWireNum() << " "
	   << thePair->GetUpper()->GetVCluster()->GetPivotWireNum() << " "
	   << thePair->GetLower()->GetUCluster()->GetPivotWireNum() << " "
	   << thePair->GetLower()->GetVCluster()->GetPivotWireNum() << " "
	   << thePair->GetError() << endl;
    }
#endif
    // Stop if track matching error too big
    if( thePair->GetError() > fErrorCutoff )
      break;

    // Get the tracks of the pair
    track   = thePair->GetLower();
    partner = thePair->GetUpper();
    if( !track || !partner ) 
      continue;

    //FIXME: debug
#ifdef WITH_DEBUG
    if( fDebug>0 ) {
      cout << "dUpper/dLower = " 
	   << thePair->GetProjectedDistance( track,partner,fSpacing) << "  "
	   << thePair->GetProjectedDistance( partner,track,-fSpacing);
    }
#endif

    // Skip pairs where any of the tracks already has a partner
    if( track->GetPartner() || partner->GetPartner() ) {
#ifdef WITH_DEBUG
      if( fDebug>0 )
	cout << " ... skipped.\n";
#endif
      continue;
    }
#ifdef WITH_DEBUG
    if( fDebug>0 )
      cout << " ... good.\n";
#endif

    // Make the tracks of this pair each other's partners. This prevents
    // tracks from being associated with more than one valid pair.
    track->SetPartner( partner );
    partner->SetPartner( track );
    thePair->SetStatus(1);

    nTracks++;

    // Compute global track values and get TRANSPORT coordinates for tracks.
    // Replace local cluster slopes with global ones, 
    // which have higher precision.

    THaVDCCluster 
      *tu = track->GetUCluster(), 
      *tv = track->GetVCluster(), 
      *pu = partner->GetUCluster(),
      *pv = partner->GetVCluster();

    Double_t du = pu->GetIntercept() - tu->GetIntercept();
    Double_t dv = pv->GetIntercept() - tv->GetIntercept();
    Double_t mu = du / fSpacing;
    Double_t mv = dv / fSpacing;

    tu->SetSlope(mu);
    tv->SetSlope(mv);
    pu->SetSlope(mu);
    pv->SetSlope(mv);

    // Recalculate the UV track's detector coordinates using the global
    // U,V slopes.
    track->CalcDetCoords();
    partner->CalcDetCoords();

#ifdef WITH_DEBUG
    if( fDebug>2 )
      cout << "Global track parameters: " 
	   << mu << " " << mv << " " 
	   << track->GetTheta() << " " << track->GetPhi()
	   << endl;
#endif

    // If the 'tracks' array was given, add THaTracks to it 
    // (or modify existing ones).
    if (tracks) {

      // Decide whether this is a new track or an old track 
      // that is being updated
      THaVDCTrackID* thisID = new THaVDCTrackID(track,partner);
      THaTrack* theTrack = NULL;
      bool found = false;
      int t;
      for( t = 0; t < n_exist; t++ ) {
	theTrack = static_cast<THaTrack*>( tracks->At(t) );
	// This test is true if an existing track has exactly the same clusters
	// as the current one (defined by track/partner)
	if( theTrack && theTrack->GetCreator() == this &&
	    *thisID == *theTrack->GetID() ) {
	  found = true;
	  break;
	}
	// FIXME: for debugging
	n_oops++;
      }

      UInt_t flag = theStage;
      if( nPairs > 1 )
	flag |= kMultiTrack;

      if( found ) {
#ifdef WITH_DEBUG
        if( fDebug>0 )
          cout << "Track " << t << " modified.\n";
#endif
        delete thisID;
        ++n_mod;
      } else {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << tracks->GetLast()+1 << " added.\n";
#endif
	theTrack = AddTrack(*tracks, 0.0, 0.0, 0.0, 0.0, thisID );
	//	theTrack->SetID( thisID );
	//	theTrack->SetCreator( this );
	theTrack->AddCluster( track );
	theTrack->AddCluster( partner );
	if( theStage == kFine ) 
	  flag |= kReassigned;
      }

      theTrack->SetD(track->GetX(), track->GetY(), track->GetTheta(), 
		     track->GetPhi());
      theTrack->SetFlag( flag );

      // calculate the TRANSPORT coordinates
      CalcFocalPlaneCoords(theTrack, kRotatingTransport);
    }
  }

#ifdef WITH_DEBUG
  if( fDebug>0 )
    cout << nTracks << " good tracks.\n";
#endif

  // Delete tracks that were not updated
  if( tracks && n_exist > n_mod ) {
    bool modified = false;
    for( int i = 0; i < tracks->GetLast()+1; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
      // Track created by this class and not updated?
      if( (theTrack->GetCreator() == this) &&
	  ((theTrack->GetFlag() & kStageMask) != theStage ) ) {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << i << " deleted.\n";
#endif
	tracks->RemoveAt(i);
	modified = true;
      }
    }
    // Get rid of empty slots - they may cause trouble in the Event class and
    // with global variables.
    // Note that the PIDinfo and vertices arrays are not reordered.
    // Therefore, PID and vertex information must always be retrieved from the
    // track objects, not from the PID and vertex TClonesArrays.
    // FIXME: Is this really what we want?
    if( modified )
      tracks->Compress();
  }

  return nTracks;
}

//_____________________________________________________________________________
Int_t THaVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes
  
  Clear();
  fLower->Decode(evdata); 
  fUpper->Decode(evdata);

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::CoarseTrack( TClonesArray& tracks )
{
 
#ifdef WITH_DEBUG
  static int nev = 0;
  if( fDebug>0 ) {
    nev++;
    cout << "=========================================\n";
    cout << "Event: " << nev << endl;
  }
#endif

  // Quickly calculate tracks in upper and lower UV planes
  fLower->CoarseTrack();
  fUpper->CoarseTrack();

  // Build tracks and mark them as level 1
  fNtracks = ConstructTracks( &tracks, 1 );

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::FineTrack( TClonesArray& tracks )
{
  // Calculate exact track position and angle using drift time information.
  // Assumes that CoarseTrack has been called (ie - clusters are matched)
  
  if( TestBit(kCoarseOnly) )
    return 0;

  fLower->FineTrack();
  fUpper->FineTrack();

  //FindBadTracks(tracks);
  //CorrectTimeOfFlight(tracks);

  // FIXME: Is angle information given to T2D converter?
  for (Int_t i = 0; i < fNumIter; i++) {
    ConstructTracks();
    fLower->FineTrack();
    fUpper->FineTrack();
  }

  fNtracks = ConstructTracks( &tracks, 2 );

#ifdef WITH_DEBUG
  // Wait for user to hit Return
  static char c;
  if( fDebug>1 ) {
    cin.clear();
    while( !cin.eof() && cin.get(c) && c != '\n');
  }
#endif

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::FindVertices( TClonesArray& tracks )
{
  // Calculate the target location and momentum at the target.
  // Assumes that CoarseTrack() and FineTrack() have both been called.

  Int_t n_exist = tracks.GetLast()+1;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks.At(t) );
    CalcTargetCoords(theTrack, kRotatingTransport);
  }

  return 0;
}

//_____________________________________________________________________________
void THaVDC::CalcFocalPlaneCoords( THaTrack* track, const ECoordTypes mode )
{
  // calculates focal plane coordinates from detector coordinates

  Double_t tan_rho, cos_rho, tan_rho_loc, cos_rho_loc;
  // TRANSPORT coordinates (projected to z=0)
  Double_t x, y, theta, phi;
  // Rotating TRANSPORT coordinates
  Double_t r_x, r_y, r_theta, r_phi;
  
  // tan rho (for the central ray) 
  // = tangent of the angle from the VDC cs to the TRANSPORT cs
  tan_rho = -fTan_vdc;
  cos_rho = 1.0/sqrt(1.0+tan_rho*tan_rho);

  // first calculate the transport frame coordinates
  theta = (track->GetDTheta()+tan_rho) /
    (1.0-track->GetDTheta()*tan_rho);
  x = track->GetDX() * cos_rho * (1.0 + theta * tan_rho);
  phi = track->GetDPhi() / (1.0-track->GetDTheta()*tan_rho) / cos_rho;
  y = track->GetDY() + tan_rho*phi*track->GetDX()*cos_rho;
  
  // then calculate the rotating transport frame coordinates
  r_x = x;

  // calculate the focal-plane matrix elements
  if(mode == kTransport)
    CalcMatrix( x, fFPMatrixElems );
  else if (mode == kRotatingTransport)
    CalcMatrix( r_x, fFPMatrixElems );

  r_y = y - fFPMatrixElems[Y000].v;  // Y000

  // Calculate now the tan(rho) and cos(rho) of the local rotation angle.
  tan_rho_loc = fFPMatrixElems[T000].v;   // T000
  cos_rho_loc = 1.0/sqrt(1.0+tan_rho_loc*tan_rho_loc);
  
  r_phi = (track->GetDPhi() - fFPMatrixElems[P000].v /* P000 */ ) / 
    (1.0-track->GetDTheta()*tan_rho_loc) / cos_rho_loc;
  r_theta = (track->GetDTheta()+tan_rho_loc) /
    (1.0-track->GetDTheta()*tan_rho_loc);

  // set the values we calculated
  track->Set(x, y, theta, phi);
  track->SetR(r_x, r_y, r_theta, r_phi);

}

//_____________________________________________________________________________
void THaVDC::CalcTargetCoords(THaTrack *track, const ECoordTypes mode)
{
  // calculates target coordinates from focal plane coordinates

  const Int_t kNUM_PRECOMP_POW = 10;

  Double_t x_fp, y_fp, th_fp, ph_fp;
  Double_t powers[kNUM_PRECOMP_POW][3];  // {th, y, ph}
  Double_t x, y, theta, phi, dp, p;

  // first select the coords to use
  if(mode == kTransport) {
    x_fp = track->GetX();
    y_fp = track->GetY();
    th_fp = track->GetTheta();
    ph_fp = track->GetPhi();
  } else if(mode == kRotatingTransport) {
    x_fp = track->GetRX();
    y_fp = track->GetRY();
    th_fp = track->GetRTheta();
    ph_fp = track->GetRPhi();  
  }

  // calculate the powers we need
  for(int i=0; i<kNUM_PRECOMP_POW; i++) {
    powers[i][0] = pow(th_fp, i);
    powers[i][1] = pow(y_fp, i);
    powers[i][2] = pow(ph_fp, i);
  }

  // calculate the matrices we need
  CalcMatrix(x_fp, fDMatrixElems);
  CalcMatrix(x_fp, fTMatrixElems);
  CalcMatrix(x_fp, fYMatrixElems);
  CalcMatrix(x_fp, fPMatrixElems);

  // calculate the coordinates at the target
  theta = CalcTargetVar(fTMatrixElems, powers);
  phi = CalcTargetVar(fPMatrixElems, powers);
  y = CalcTargetVar(fYMatrixElems, powers);

  // calculate momentum
  dp = CalcTargetVar(fDMatrixElems, powers);
  p  = static_cast<THaSpectrometer*>(fApparatus)->GetPcentral() * (1.0+dp);

  //FIXME: estimate x ??
  x = 0.0;

  // Save the target quantities with the tracks
  track->SetTarget(x, y, theta, phi);
  track->SetDp(dp);
  track->SetMomentum(p);
  static_cast<THaSpectrometer*>(fApparatus)->
    TransportToLab( p, theta, phi, track->GetPvect() );

}


//_____________________________________________________________________________
void THaVDC::CalcMatrix( const Double_t x, vector<THaMatrixElement>& matrix )
{
  // calculates the values of the matrix elements for a given location
  // by evaluating a polynomial in x of order it->order with 
  // coefficients given by it->poly

  for( vector<THaMatrixElement>::iterator it=matrix.begin();
       it!=matrix.end(); it++ ) {
    it->v = 0.0;

    if(it->iszero == false) {
      for(int i=it->order-1; i>=1; i--)
	it->v = x * (it->v + it->poly[i]);
      it->v += it->poly[0];
    }
  }
}

//_____________________________________________________________________________
Double_t THaVDC::CalcTargetVar(const vector<THaMatrixElement>& matrix,
			       const Double_t powers[][3])
{
  // calculates the value of a variable at the target

  Double_t retval=0.0;
  for( vector<THaMatrixElement>::const_iterator it=matrix.begin();
       it!=matrix.end(); it++ ) 
    if(it->v != 0.0)
      retval += it->v * powers[it->pw[0]][0]
	              * powers[it->pw[1]][1]
	              * powers[it->pw[2]][2];

  return retval;
}

//_____________________________________________________________________________
void THaVDC::CorrectTimeOfFlight(TClonesArray& tracks)
{
  const static Double_t v = 3.0e-8;   // for now, assume that everything travels at c

  if( (!fS1 == NULL) || (fS2 == NULL) )
    return;

  THaScintillator* s1 = static_cast<THaScintillator*>(fS1);
  THaScintillator* s2 = static_cast<THaScintillator*>(fS2);

  // adjusts caluculated times so that the time of flight to S1
  // is the same as a track going through the middle of the VDC
  // (i.e. x_det = 0) at a 45 deg angle (theta_t and phi_t = 0)
  // assumes that at least the coarse tracking has been performed

  Int_t n_exist = tracks.GetLast()+1;
  //cerr<<"num tracks: "<<n_exist<<endl;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* track = static_cast<THaTrack*>( tracks.At(t) );
    
    // calculate the correction, since it's on a per track basis
    Double_t s1_dist, vdc_dist, dist, tdelta, central_dist;
    if(!s1->CalcPathLen(track, s1_dist))
      s1_dist = 0.0;
    if(!CalcPathLen(track, vdc_dist))
      vdc_dist = 0.0;

    // since the z=0 of the transport coords is inclined with respect
    // to the VDC plane, the VDC correction depends on the location of
    // the track
    if( track->GetX() < 0 )
      dist = s1_dist + vdc_dist;
    else
      dist = s1_dist - vdc_dist;
    
    // figure out the track length from the origin to the s1 plane
    // since we take the VDC to be the origin of the coordinate
    // space, this is actually pretty simple
    central_dist = fS1->GetOrigin().Z();

    tdelta = ( central_dist - dist) / v;
    //cout<<"time correction: "<<tdelta<<endl;

    // apply the correction
    Int_t n_clust = track->GetNclusters();
    for( Int_t i = 0; i < n_clust; i++ ) {
      THaVDCUVTrack* the_uvtrack = 
	static_cast<THaVDCUVTrack*>( track->GetCluster(i) );
      if( !the_uvtrack )
	continue;
      
      //FIXME: clusters guaranteed to be nonzero?
      the_uvtrack->GetUCluster()->SetTimeCorrection(tdelta);
      the_uvtrack->GetVCluster()->SetTimeCorrection(tdelta);
    }
  }
}

//_____________________________________________________________________________
void THaVDC::FindBadTracks(TClonesArray& tracks)
{
  // Flag tracks that don't intercept S2 scintillator as bad

  THaScintillator* s2 = static_cast<THaScintillator*>(fS2);

  if(s2 == NULL) {
    //cerr<<"Could not find s2 plane!!"<<endl;
    return;
  }

  Int_t n_exist = tracks.GetLast()+1;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* track = static_cast<THaTrack*>( tracks.At(t) );
    Double_t x2, y2;

    // project the current x and y positions into the s2 plane
    if(!s2->CalcInterceptCoords(track, x2, y2)) {
      x2 = 0.0;
      y2 = 0.0;
    } 

    // if the tracks go out of the bounds of the s2 plane,
    // toss the track out
    if( (TMath::Abs(x2 - s2->GetOrigin().X()) > s2->GetSize()[0]) ||
	(TMath::Abs(y2 - s2->GetOrigin().Y()) > s2->GetSize()[1]) ) {

      // for now, we just flag the tracks as bad
      track->SetFlag( track->GetFlag() | kBadTrack );

      //tracks.RemoveAt(t);
#ifdef WITH_DEBUG
      //cout << "Track " << t << " deleted.\n";
#endif  
    }
  }

  // get rid of the slots for the deleted tracks
  //tracks.Compress();
}

//_____________________________________________________________________________
void THaVDC::Draw(TGeometry* geom, Option_t *opt)
{

  TVector3 loc = GetOrigin();

  Double_t orig[3] = { loc.x() , loc.y() ,loc.z() };
	  
  printf("VDC Detector %s at %f,%f,%f \n",GetName(),orig[0],orig[1],orig[2]);
  printf("VDC Detector %s is %f x %f x %f \n",GetName(),fSize[0],fSize[1],fSize[2]);
	  //
	  //              // fSize[1],[2] in half-widths, [3] in full width.
  TBRIK* b = new TBRIK(GetName(),"TITLE","void",fSize[0],fSize[1],fSize[2]/2);
  b->SetLineColor(atoi(opt));
	//
  TRotMatrix* rot = NULL;
  if(!(rot = geom->GetRotMatrix("XZ")))
  {
    //only Geant-like constructor is implemented.
    printf("Missing rotmatrix XZ.\n");
  }
  
  //Front plane at z coord.
  geom->Node(GetTitle(),GetTitle(),GetName(),orig[0],orig[1],orig[2]+(fSize[2]/2),"XZ");
  
	  //
  fGraphics.Add(b);
	  //                          
  

  

}
//_____________________________________________________________________________

void THaVDC::Draw(TGeometry* geom, THaTrack* track,Double_t& t, Option_t *opt)
{

  Double_t x=0,y = 0;

  if(CalcInterceptCoords(track,x,y))
  {

     TVector3 loc = GetOrigin(); // FIXME: z is not right. 

     /*
     TSPHE* p = new TSPHE("HIT","Hit on detector","void",.05);
     fGraphics.Add(p);

     geom->Node("HITNODE","Node for hit","HIT",x*TMath::Cos(TMath::Pi()/4),y,x*TMath::Sin(TMath::Pi()/4));
     */

     TVector3 trackdir;
     GetTrackDir(track,&trackdir);
     DrawLine(geom,x*TMath::Cos(TMath::Pi()/4),y,x*TMath::Sin(TMath::Pi()/4),4,trackdir);   
  
  
  }
  
}
//_____________________________________________________________________________
void THaVDC::Draw(TGeometry* geom, const THaEvData& evdata,const Option_t *opt)
{

  THaVDCUVPlane* lower = GetLower();
  lower->GetUPlane()->Draw(geom,evdata);
  lower->GetVPlane()->Draw(geom,evdata);

  THaVDCUVPlane* upper = GetUpper();
  upper->GetUPlane()->Draw(geom,evdata);
  upper->GetVPlane()->Draw(geom,evdata);     

}

//_____________________________________________________________________________
void THaVDC::DrawLine(TGeometry* geom, Double_t x, Double_t y, Double_t z,Double_t len,TVector3& dir)
{
  if(!geom->GetShape("LINE"))
  {
    TBRIK* b = new TBRIK("LINE","Track projection.","void",0,0,len/2);
    fGraphics.Add(b);
  }
  

  Double_t px,py,pz,tx,ty,tz;

  tx = 90 + (dir.Theta()*180/PI);
  px = dir.Phi()*180/PI;
  ty = 90 + dir.Theta()*180/PI;
  py = 90 + dir.Phi()*180/PI;
  tz = dir.Theta()*180/PI;
  pz = dir.Phi()*180/PI;
  

  TString rotid = "TRACK";
  rotid += gNum++;
	  
  TRotMatrix* rot = new TRotMatrix(rotid,"TRACK",tx,
			  px,
			  ty,
			  py,
			  tz,
			  pz);
		  

  geom->Node("HITLINE","HITLINE","LINE",x+2*dir.X(),y+2*dir.Y(),z+2*dir.Z(),rotid);
}

//_____________________________________________________________________________
/*
bool THaVDC::CalcTrackIntercept(THaTrack* theTrack, Double_t& t,Double_t& xcross,Double_t& ycross)
{
	//Calculates the x,y coords in the transport for the angled VDC.
	//
	
  TVector3 t0( theTrack->GetX(), theTrack->GetY(), 0.0 );
  Double_t norm = TMath::Sqrt(1.0 + theTrack->GetTheta()*theTrack->GetTheta() +
     theTrack->GetPhi()*theTrack->GetPhi());

  TVector3 t_hat( theTrack->GetTheta()/norm, theTrack->GetPhi()/norm, 1.0/norm )
	      ;
  fDenom.SetColumn( -t_hat, 2 );
  fNom.SetColumn( t0-fOrigin, 2 );

	  // first get the distance...
  Double_t det = fDenom.Determinant();
  if( fabs(det) < 1e-5 )
    return false;  // No useful solution for this track
  t = fNom.Determinant() / det;
//
  // ...then the intersection point
  TVector3 v = t0 + t*t_hat - fOrigin;
  xcross = v.Dot(fXax);
  ycross = v.Dot(fYax);

  return true;
	  //                     
*/

void THaVDC::DefineAxes(Double_t rotation_angle)
{
  fXax.SetXYZ( TMath::Cos(TMath::Pi()/4),0, TMath::Sin(TMath::Pi()/4) );
  fYax.SetXYZ( 0.0, 1, 0.0);
  fZax = fXax.Cross(fYax);

   //cout<<"Z: "<<fZax.X()<<" "<<fZax.Y()<<" "<<fZax.Z()<<endl;
  cout << "VDC angle: " << fVDCAngle << endl;
  cout << "rotation angle: " << rotation_angle << endl;

  fDenom.ResizeTo( 3, 3 );
  fNom.ResizeTo( 3, 3 );
  fDenom.SetColumn( fXax, 0 );
  fDenom.SetColumn( fYax, 1 );
  fNom.SetColumn( fXax, 0 );
  fNom.SetColumn( fYax, 1 );
}
            
////////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDC)
