#ifndef ROOT_THaPolyLine_h
#define ROOT_THaPolyLine_h
//
//
//

#include "TPaveText.h"
#include "TPolyLine.h"
#include "TCanvas.h"

class THaPolyLine : public TPolyLine
{

 public:
  THaPolyLine(Int_t n, Double_t* x, Double_t* y, Option_t* option = NULL);
  virtual ~THaPolyLine();

  virtual void          ExecuteEvent(Int_t event, Int_t px, Int_t py);

  void                  SetText(const char* text,TCanvas* canvas);
  
 protected:

  TString*              fText;
  TPaveText*            fPave;
  TCanvas*              fcanvas;

  void                  ShowToolTip();
  void                  HideToolTip();

  ClassDef(THaPolyLine,0);

};



#endif
