#ifndef ROOT_THaETVDCGrDetail
#define ROOT_THaETVDCGrDetail

//////////////////////////////////////////////////////
//
//  THaETVDCGrDetail
//
//////////////////////////////////////////////////////

#include "THaETDetail.h"
#include "THaVDC.h"
#include "TCanvas.h"

class THaETVDCGrDetail : public THaETDetail {

 public:

  THaETVDCGrDetail(THaVDC* vdc, const char* name, const char* title = NULL,const char* Opt = "3");
  virtual ~THaETVDCGrDetail();

  virtual Int_t     Process(const THaEvData& evdata);
  virtual THaAnalysisObject::EStatus   Init();

 protected:

  THaVDC*           fVDC;
  TCanvas*          fCanvas;
  TString           fOpt;

  virtual void      Clear();
 private:

  

  ClassDef(THaETVDCGrDetail,0)
};

#endif

