//*-- Author : 	Robert Stringer	16-Jul-03
//
//////////////////////////////////////////////////////
//
// THaETVDCGrDetail
//
// EventTrack Detail window to show graphs of wire number 
// vs. drift time for each vdc plane.
//
/////////////////////////////////////////////////////

#include "THaETVDCGrDetail.h"

//----------------------------------------------------

THaETVDCGrDetail::THaETVDCGrDetail(THaVDC* vdc,const char* name, const char* title)
{
  fVDC = vdc;
  fName = name;
  fTitle = title;
}

//----------------------------------------------------
THaETVDCGrDetail::~THaETVDCGrDetail()
{

  if(fCanvas)
    delete fCanvas;

}

//----------------------------------------------------

Int_t THaETVDCGrDetail::Process(const THaEvData& evdata)
{
  //Process current event

  fCanvas->SetEditable(kTRUE);
  Clear();

  fVDC->DrawGraph(fCanvas);
  
  fCanvas->Update();

  fCanvas->SetEditable(kFALSE);
  
  return 0;
}

//----------------------------------------------------

THaAnalysisObject::EStatus THaETVDCGrDetail::Init()
{
  //Init Detail window

  fCanvas = new TCanvas(fName,fTitle);

  
}

//----------------------------------------------------
void THaETVDCGrDetail::Clear()
{
  // Clear the canvas but do not delete the pads.
  fCanvas->Clear();

}

ClassImp(THaETVDCGrDetail)
