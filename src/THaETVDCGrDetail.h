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

  THaETVDCGrDetail(THaVDC* vdc, const char* name, const char* title = NULL);
  virtual ~THaETVDCGrDetail();

  virtual Int_t     Process(const THaEvData& evdata);
  virtual THaAnalysisObject::EStatus   Init();

 protected:

  THaVDC*           fVDC;
  TCanvas*          fCanvas;

  virtual void      Clear();
 private:

  ClassDef(THaETVDCGrDetail,0)
};

#endif

