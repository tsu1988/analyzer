#ifndef ROOT_THaETVDCDetail
#define ROOT_THaETVDCDetail

//////////////////////////////////////////////////////
//
//  THaETVDCDetail
//
//////////////////////////////////////////////////////

#include "THaETDetail.h"
#include "THaVDC.h"
#include "TCanvas.h"

class THaETVDCDetail : public THaETDetail {

 public:

  THaETVDCDetail(THaVDC* vdc, const char* name, const char* title = NULL);
  virtual ~THaETVDCDetail();

  virtual Int_t     Process(const THaEvData& evdata);
  virtual THaAnalysisObject::EStatus   Init();

 protected:

  THaVDC*           fVDC;
  TCanvas*          fCanvasU;
  TCanvas*          fCanvasV;

  //  virtual void      DrawLocalFit(THaVDCPlane* plane,TCanvas* canvas, Double_t min,Double_t max,Double_t x, Double_t y);
  virtual void      Clear();
 private:

  ClassDef(THaETVDCDetail,0)
};

#endif

