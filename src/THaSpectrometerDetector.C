//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"
#include "THaTrack.h"
#include "TBRIK.h"
#include "TSPHE.h"
#include "TNode.h"

#define PI 3.14159

ClassImp(THaSpectrometerDetector)

//______________________________________________________________________________
THaSpectrometerDetector::THaSpectrometerDetector( const char* name, 
						  const char* description,
						  THaApparatus* apparatus )
  : THaDetector(name,description,apparatus)
{
  // Constructor

}

//______________________________________________________________________________
THaSpectrometerDetector::~THaSpectrometerDetector()
{
  // Destructor
  TIter itr(&fGraphics);
  TObject *t;
  while(t = itr.Next())
  {
    printf("Freed.\n");
    delete t;
  }
	     
}

//_____________________________________________________________________________
void THaSpectrometerDetector::DefineAxes(Double_t rotation_angle)
{
  // define variables used for calculating intercepts of tracks
  // with the detector
  // right now, we assume that all detectors except VDCs are 
  // perpendicular to the Transport frame

  fXax.SetXYZ( TMath::Cos(rotation_angle), 0.0, TMath::Sin(rotation_angle) );
  fYax.SetXYZ( 0.0, 1.0, 0.0 );
  fZax = fXax.Cross(fYax);

}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CalcTrackIntercept(THaTrack* theTrack, 
					 Double_t& t, Double_t& xcross, 
					 Double_t& ycross)
{
  // projects a given track on to the plane of the detector
  // xcross and ycross are the x and y coords of this intersection
  // t is the distance from the origin of the track to the given plane

  TVector3 t0( theTrack->GetX(), theTrack->GetY(), 0.0 );
  Double_t norm = TMath::Sqrt(1.0 + theTrack->GetTheta()*theTrack->GetTheta() +
			      theTrack->GetPhi()*theTrack->GetPhi());
  TVector3 t_hat( theTrack->GetTheta()/norm, theTrack->GetPhi()/norm, 1.0/norm );

  TVector3 v;
  if( !IntersectPlaneWithRay( fXax, fYax, fOrigin, t0, t_hat, t, v ))
    return false;
  v -= fOrigin;
  xcross = v.Dot(fXax);
  ycross = v.Dot(fYax);
  
  //cout << "t0: " << t0.X()<< "," << t0.Y() << "," << t0.Z() << endl;
  //cout << "fOrigin: " << fOrigin.X() << "," << fOrigin.Y() << "," << fOrigin.Z()<< endl;
	  
  return true;
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CheckIntercept(THaTrack *track)
{
  Double_t x, y, t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CalcInterceptCoords(THaTrack* track, Double_t& x, Double_t& y)
{
  Double_t t;
  return CalcTrackIntercept(track, t, x, y);
}

//_____________________________________________________________________________
bool THaSpectrometerDetector::CalcPathLen(THaTrack* track, Double_t& t)
{
  Double_t x, y;
  return CalcTrackIntercept(track, t, x, y);
}
//____________________________________________________________________________
void THaSpectrometerDetector::Draw(const Option_t* opt)
{
	//Draw something?
}
//____________________________________________________________________________
void THaSpectrometerDetector::Draw(TGeometry* geom, const THaEvData& evdata, const Option_t* opt)
{
  
}	
//____________________________________________________________________________
void THaSpectrometerDetector::Draw(TGeometry* geom, const Option_t* opt)
{
  //Draw detector geometry.
	 
  TVector3 loc = GetOrigin();
  
//  Double_t x1 = (Double_t) loc.z() * centralangle.CosTheta() - fSize[0];
//  Double_t y1 = (Double_t) loc.z() * sin(centralangle.Theta()) - fSize[1];
//  Double_t x2 = (Double_t) loc.z() * centralangle.CosTheta() + fSize[0];
//  Double_t y2 = (Double_t) loc.z() * sin(centralangle.Theta()) + fSize[1];

  Double_t orig[3] = { loc.x() , loc.y() ,loc.z() };
 
  printf("Detector %s at %f,%f,%f \n",GetName(),orig[0],orig[1],orig[2]);
    printf("Detector %s is %f x %f x %f \n",GetName(),fSize[0],fSize[1],fSize[2]); 

    // fSize[1],[2] in half-widths, [3] in full width.
  TBRIK* b = new TBRIK(GetName(),"TITLE","void",fSize[0],fSize[1],fSize[2]/2);
  b->SetLineColor(atoi(opt));

    //Front plane at z coord.
  geom->Node(GetTitle(),GetTitle(),GetName(),orig[0],orig[1],orig[2]+(fSize[2]/2),"ZERO");

  fGraphics.Add(b);

  //b->SetFillColor(0);
  //b->SetFillStyle(0);
  //b->Draw();

}
void THaSpectrometerDetector::Draw(TGeometry* geom, THaTrack* track, Double_t& t, const Option_t* opt)
{

/* Old code for plotting track on detectors.
	
  //Draw hits
  Double_t x=0,y = 0;

  if(CalcInterceptCoords(track,x,y))
  {
    TVector3 loc = GetOrigin();
    
    TSPHE* p = new TSPHE("HIT","Hit on detector","void",.05);
    fGraphics.Add(p);
    
    geom->Node("HITNODE","Node for hit","HIT",x,y,loc.z());
  }
        
*/
		      

}
bool THaSpectrometerDetector::GetTrackDir(THaTrack* theTrack, TVector3* dir)
{
  //Get track unit vector.
  
  TVector3 t( theTrack->GetTheta(), theTrack->GetPhi(), 1);

  Double_t norm = TMath::Sqrt(1.0 + TMath::Power((Double_t) 
	theTrack->GetTheta(),2) + TMath::Power((Double_t) theTrack->GetPhi(),2));

  if(norm == 0)
	  return false;
  else
  {
    t *= (Double_t) (1/ norm);
    *dir = t;
  }
  return true;
}


