
#include "TPaveText.h"
#include "TPolyLine.h"
#include "THaPolyLine.h"
#include "iostream.h"
#include "Buttons.h"
#include "TVirtualPad.h"

THaPolyLine::THaPolyLine(Int_t n, Double_t* x, Double_t* y, Option_t* option):TPolyLine(n,x,y,option)
{
  //Calls default constructor
  fText = NULL; 
}


THaPolyLine::~THaPolyLine()
{

  if(fText)
    delete fText;

}
void THaPolyLine::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{

  switch(event)
    {
      case kMouseEnter:
	ShowToolTip();

      break;

      case kMouseLeave:
	HideToolTip();

	break;
    
      default:
         TPolyLine::ExecuteEvent(event, px, py);
	 break;
    }
}

void THaPolyLine::ShowToolTip()
{
  fPave = new TPaveText(.1,.3,.5,.5);
  TText* t1 = fPave->AddText(*fText);

  fcanvas->cd();
  fPave->SetFillColor(6);
  fPave->Draw();
  fcanvas->Update();
}

void THaPolyLine::HideToolTip()
{

  delete fPave;
  fcanvas->Update();
}

void THaPolyLine::SetText(const char* text,TCanvas* canvas)
{
  cout << "ftext = " << fText << endl;
  cout << "text = " << text << endl;

  fText = new TString(text);
  fcanvas = canvas; 
}

ClassImp(THaPolyLine)
