//*-- Author : 	Robert Stringer	17-Jul-03
//
//////////////////////////////////////////////////////
//
// THaETVDCDetail
//
// EventTrack Detail window to show drift distances per wire  
// for each vdc plane.
//
/////////////////////////////////////////////////////

#include "THaETVDCDetail.h"
#include "THaAnalysisObject.h"
#include "TCanvas.h"
#include "THaVDCCluster.h"
#include "TPolyLine.h"
#include "THaVDCUVPlane.h"
#include "THaVDCPlane.h"

//----------------------------------------------------

THaETVDCDetail::THaETVDCDetail(THaVDC* vdc,const char* name, const char* title)
{
  fVDC = vdc;
  fName = name;
  fTitle = title;

  Init();
  
}

//----------------------------------------------------
THaETVDCDetail::~THaETVDCDetail()
{

  if(fCanvasU)
    delete fCanvasU;
  if(fCanvasV)
    delete fCanvasV;

}

//----------------------------------------------------

Int_t THaETVDCDetail::Process(const THaEvData& evdata)
{
  //Process current event
  Clear();

  fVDC->DrawPlanes(fCanvasU,'U');
  fVDC->DrawPlanes(fCanvasV,'V');
  return 0;
}

//----------------------------------------------------

THaAnalysisObject::EStatus THaETVDCDetail::Init()
{
  //Init Detail window

  fCanvasU = new TCanvas("UPlane","VDC Detail - U");
  fCanvasV = new TCanvas("VPlane","VDC Detail - V");

  Double_t uscale = fVDC->DrawPlanes(fCanvasU,'U');
  Double_t vscale = fVDC->DrawPlanes(fCanvasV,'V');
  
  THaVDCUVPlane* upper = fVDC->GetUpper();
  THaVDCUVPlane* lower = fVDC->GetLower();

  return THaAnalysisObject::kOK;
}

//----------------------------------------------------
void THaETVDCDetail::Clear()
{
  // Clear the canvas but do not delete the pads.
  fCanvasU->Clear("D");
  fCanvasV->Clear("D");

  fCanvasU->Clear();
  fCanvasV->Clear();
  
}
//----------------------------------------------------
/*
void THaETVDCDetail::DrawLocalFit(THaVDCPlane* plane,TCanvas* canvas,Double_t min, Double_t max,Double_t x, Double_t y)
{

  const Float_t* VDCSize = fVDC->GetSize();
  THaVDC* VDC = static_cast<THaVDC*>(fVDC);
  Double_t VDCPlSpacing = VDC->GetSpacing();
  Double_t x_scale = VDCSize[0]/TMath::Sin(45*TMath::Pi()/180) / Double_t( max - min );
  Double_t y_scale = .7/VDCPlSpacing;

  y += (plane->fZ) * y_scale;


  TClonesArray* clusters(plane->GetClusters());
  Int_t last = clusters->GetLast();
  cout << "nclusters = " << last+1 << endl;
  THaVDCCluster* cluster;
  if(last+1>0 && clusters->At(0)) {
    cluster = (THaVDCCluster*)clusters->At(0);
    cluster->Print();
  }
  TIter next(clusters);

  while(cluster = static_cast<THaVDCCluster*>( next() ))
    {
      Double_t intercept = cluster->GetIntercept();
      Double_t slope = cluster->GetSlope();

      canvas->cd();

      Double_t x[2],y[2];

      x[0] = x + (intercept-min)/(VDCSize[0]*2)*x_scale - .1;
      y[0] = y + x[0]*slope*y_scale;;

      x[1] = x + (intercept-min)/(VDCSize[0]*2)*x_scale + .1;
      y[1] = y + x[1]*slope*y_scale;

      //Fix: Free this.
      TPolyLine* line = new TPolyLine(2,x,y);
      
      line->Draw();

    }

}
*/

ClassImp(THaETVDCDetail)
