/////////////////////////////
//
//
//  New TGraph allowing colored points
//
//
////////////////////////////


#include "TVirtualX.h"
#include "TPolyMarker.h"
#include "TVirtualPad.h"
#include "TROOT.h"
#include "TStyle.h"
#include "THaGraph.h"
#include "iostream.h"

Double_t *gxwork, *gywork, *gxworkl, *gyworkl;

THaGraph::THaGraph(Int_t n, const Double_t* x, const Double_t* y, Int_t* color):TGraph(n,x,y)
{
  
  fcolor = new Int_t[n];

  for(Int_t x = 0; x < n; x++)
    fcolor[x] = color[x];

}

THaGraph::~THaGraph()
{
  delete fcolor;
}


void THaGraph::PaintGraph(Int_t npoints, const Double_t* x, const Double_t* y, Option_t* chopt)
{
//*-*-*-*-*-*-*-*-*-*-*-*Control function to draw a graph*-*-*-*-*-*-*-*-*-*-*
//*-*                    ================================
//
//
//  Draws one dimensional graphs. The aspect of the graph is done
//  according to the value of the chopt.
//
//  Input parameters:
//
//  npoints : Number of points in X or in Y.
//  x[npoints] or x[2] : X coordinates or (XMIN,XMAX) (WC space).
//  y[npoints] or y[2] : Y coordinates or (YMIN,YMAX) (WC space).
//  chopt : Option.
//
//  chopt='L' :  A simple polyline between every points is drawn
//
//  chopt='F' :  A fill area is drawn ('CF' draw a smooth fill area)
//
//  chopt='A' :  Axis are drawn around the graph
//
//  chopt='C' :  A smooth Curve is drawn
//
//  chopt='*' :  A Star is plotted at each point
//
//  chopt='P' :  Idem with the current marker
//
//  chopt='B' :  A Bar chart is drawn at each point
//
//  chopt='1' :  ylow=rwymin
//
//


   Int_t OptionLine , OptionAxis , OptionCurve,OptionStar ,OptionMark;
   Int_t OptionBar  , OptionR    , OptionOne;
   Int_t OptionFill , OptionZ    , OptionCurveFill;
   Int_t i, npt, nloop;
   Int_t drawtype=0;
   Double_t xlow, xhigh, ylow, yhigh;
   Double_t barxmin, barxmax, barymin, barymax;
   Double_t uxmin, uxmax;
   Double_t x1, xn, y1, yn;
   Double_t dbar, bdelta;

//*-* ______________________________________

  if (npoints <= 0) {
     Error("PaintGraph", "illegal number of points (%d)", npoints);
     return;
  }
   TString opt = chopt;
   opt.ToUpper();

   if(opt.Contains("L")) OptionLine = 1;  else OptionLine = 0;
   if(opt.Contains("A")) OptionAxis = 1;  else OptionAxis = 0;
   if(opt.Contains("C")) OptionCurve= 1;  else OptionCurve= 0;
   if(opt.Contains("*")) OptionStar = 1;  else OptionStar = 0;
   if(opt.Contains("P")) OptionMark = 1;  else OptionMark = 0;
   if(opt.Contains("B")) OptionBar  = 1;  else OptionBar  = 0;
   if(opt.Contains("R")) OptionR    = 1;  else OptionR    = 0;
   if(opt.Contains("1")) OptionOne  = 1;  else OptionOne  = 0;
   if(opt.Contains("F")) OptionFill = 1;  else OptionFill = 0;
   OptionZ    = 0;
//*-*           If no "drawing" option is selected and if chopt<>' '
//*-*           nothing is done.

  if (OptionLine+OptionFill+OptionCurve+OptionStar+OptionMark+OptionBar == 0) {
     if (strlen(chopt) == 0)  OptionLine=1;
     else   return;
  }

  if (OptionStar) SetMarkerStyle(3);

  OptionCurveFill = 0;
  if (OptionCurve && OptionFill) {
     OptionCurveFill = 1;
     OptionFill      = 0;
  }


//*-*-           Draw the Axis with a fixed number of division: 510

  Double_t rwxmin,rwxmax, rwymin, rwymax, maximum, minimum, dx, dy;

  if (OptionAxis) {

     if (fHistogram) {
        rwxmin    = gPad->GetUxmin();
        rwxmax    = gPad->GetUxmax();
        rwymin    = gPad->GetUymin();
        rwymax    = gPad->GetUymax();
        minimum   = fHistogram->GetMinimumStored();
        maximum   = fHistogram->GetMaximumStored();
        if (minimum == -1111) { //this can happen after unzooming
           minimum = fHistogram->GetYaxis()->GetXmin();
           fHistogram->SetMinimum(minimum);
        }
        if (maximum == -1111) {
           maximum = fHistogram->GetYaxis()->GetXmax();
           fHistogram->SetMaximum(maximum);
        }
        uxmin     = gPad->PadtoX(rwxmin);
        uxmax     = gPad->PadtoX(rwxmax);
     } else {
        rwxmin = rwxmax = x[0];
        rwymin = rwymax = y[0];
        for (i=1;i<npoints;i++) {
           if (x[i] < rwxmin) rwxmin = x[i];
           if (x[i] > rwxmax) rwxmax = x[i];
           if (y[i] < rwymin) rwymin = y[i];
           if (y[i] > rwymax) rwymax = y[i];
        }

        ComputeRange(rwxmin, rwymin, rwxmax, rwymax);  //this is redefined in TGraphErrors

        if (rwxmin == rwxmax) rwxmax += 1.;
        if (rwymin == rwymax) rwymax += 1.;
        dx = 0.1*(rwxmax-rwxmin);
        dy = 0.1*(rwymax-rwymin);
        uxmin    = rwxmin - dx;
        uxmax    = rwxmax + dx;
        minimum  = rwymin - dy;
        maximum  = rwymax + dy;
     }
     if (fMinimum != -1111) rwymin = minimum = fMinimum;
     if (fMaximum != -1111) rwymax = maximum = fMaximum;
     if (uxmin < 0 && rwxmin >= 0) {
        if (gPad->GetLogx()) uxmin = 0.9*rwxmin;
        else                 uxmin = 0;
     }
     if (uxmax > 0 && rwxmax <= 0) {
        if (gPad->GetLogx()) uxmax = 1.1*rwxmax;
        else                 uxmax = 0;
     }
     if (minimum < 0 && rwymin >= 0) {
        if(gPad->GetLogy()) minimum = 0.9*rwymin;
        else                minimum = 0;
     }
     if (maximum > 0 && rwymax <= 0) {
        //if(gPad->GetLogy()) maximum = 1.1*rwymax;
        //else                maximum = 0;
     }
     if (minimum <= 0 && gPad->GetLogy()) minimum = 0.001*maximum;
     if (uxmin <= 0 && gPad->GetLogx()) {
        if (uxmax > 1000) uxmin = 1;
        else              uxmin = 0.001*uxmax;
     }
     rwymin = minimum;
     rwymax = maximum;

//*-*-  Create a temporary histogram and fill each channel with the function value
   if (!fHistogram) {
      // the graph is created with at least as many channels as there are points
      // to permit zooming on the full range
      rwxmin = uxmin;
      rwxmax = uxmax;
      npt = 100;
      if (fNpoints > npt) npt = fNpoints;
      fHistogram = new TH1F(GetName(),GetTitle(),npt,rwxmin,rwxmax);
      if (!fHistogram) return;
      fHistogram->SetMinimum(rwymin);
      fHistogram->SetBit(TH1::kNoStats);
      fHistogram->SetMaximum(rwymax);
      fHistogram->GetYaxis()->SetLimits(rwymin,rwymax);
      fHistogram->SetDirectory(0);
   }
   fHistogram->Paint("a");
  }

  //*-*  Set Clipping option
  gPad->SetBit(kClipFrame, TestBit(kClipFrame));

  TF1 *fit = 0;
  if (fFunctions) fit = (TF1*)fFunctions->First();
  TObject *f;
  if (fFunctions) {
     TIter   next(fFunctions);
     while ((f = (TObject*) next())) {
        if (f->InheritsFrom(TF1::Class())) {
           fit = (TF1*)f;
           break;
        }
     }
  }
  if (fit) PaintFit(fit);

  rwxmin   = gPad->GetUxmin();
  rwxmax   = gPad->GetUxmax();
  rwymin   = gPad->GetUymin();
  rwymax   = gPad->GetUymax();
  uxmin    = gPad->PadtoX(rwxmin);
  uxmax    = gPad->PadtoX(rwxmax);
  if (fHistogram) {
     maximum = fHistogram->GetMaximum();
     minimum = fHistogram->GetMinimum();
  } else {
     maximum = gPad->PadtoY(rwymax);
     minimum = gPad->PadtoY(rwymin);
  }

//*-*-           Set attributes
  TAttLine::Modify();
  TAttFill::Modify();
  TAttMarker::Modify();

//*-*-           Draw the graph with a polyline or a fill area

  gxwork  = new Double_t[2*npoints+10];
  gywork  = new Double_t[2*npoints+10];
  gxworkl = new Double_t[2*npoints+10];
  gyworkl = new Double_t[2*npoints+10];

  if (OptionLine || OptionFill) {
     x1       = x[0];
     xn       = x[npoints-1];
     y1       = y[0];
     yn       = y[npoints-1];
     nloop = npoints;
     if (OptionFill && (xn != x1 || yn != y1)) nloop++;
     npt = 0;
     for (i=1;i<=nloop;i++) {
        if (i > npoints) {
           gxwork[npt] = gxwork[0];  gywork[npt] = gywork[0];
        } else {
           gxwork[npt] = x[i-1];      gywork[npt] = y[i-1];
           npt++;
        }
        if (i == nloop) {
           ComputeLogs(npt, OptionZ);
           Int_t bord = gStyle->GetDrawBorder();
           if (OptionR) {
              if (OptionFill) {
                 gPad->PaintFillArea(npt,gyworkl,gxworkl);
                 if (bord) gPad->PaintPolyLine(npt,gyworkl,gxworkl);
              }
              else         gPad->PaintPolyLine(npt,gyworkl,gxworkl);
           }
           else {
              if (OptionFill) {
                 gPad->PaintFillArea(npt,gxworkl,gyworkl);
                 if (bord) gPad->PaintPolyLine(npt,gxworkl,gyworkl);
              }
              else         gPad->PaintPolyLine(npt,gxworkl,gyworkl);
           }
           gxwork[0] = gxwork[npt-1];  gywork[0] = gywork[npt-1];
           npt      = 1;
        }
     }
  }

//*-*-           Draw the graph with a smooth Curve. Smoothing via Smooth

  if (OptionCurve) {
     x1 = x[0];
     xn = x[npoints-1];
     y1 = y[0];
     yn = y[npoints-1];
     drawtype = 1;
     nloop = npoints;
     if (OptionCurveFill) {
        drawtype += 1000;
        if (xn != x1 || yn != y1) nloop++;
     }
     if (!OptionR) {
        npt = 0;
        for (i=1;i<=nloop;i++) {
           if (i > npoints) {
              gxwork[npt] = gxwork[0];  gywork[npt] = gywork[0];
           } else {
              if (y[i-1] < minimum || y[i-1] > maximum) continue;
              if (x[i-1] < uxmin    || x[i-1] > uxmax)  continue;
              gxwork[npt] = x[i-1];      gywork[npt] = y[i-1];
              npt++;
           }
           ComputeLogs(npt, OptionZ);
           if (gyworkl[npt-1] < rwymin || gyworkl[npt-1] > rwymax) {
              if (npt > 2) {
                 ComputeLogs(npt, OptionZ);
                 Smooth(npt,gxworkl,gyworkl,drawtype);
              }
              gxwork[0] = gxwork[npt-1]; gywork[0] = gywork[npt-1];
              npt=1;
              continue;
           }
        }  //endfor (i=0;i<nloop;i++)
        if (npt > 1) {
           ComputeLogs(npt, OptionZ);
           Smooth(npt,gxworkl,gyworkl,drawtype);
        }
     }
     else {
        drawtype += 10;
        npt    = 0;
        for (i=1;i<=nloop;i++) {
           if (i > npoints) {
              gxwork[npt] = gxwork[0];  gywork[npt] = gywork[0];
           } else {
              if (y[i-1] < minimum || y[i-1] > maximum) continue;
              if (x[i-1] < uxmin    || x[i-1] > uxmax)  continue;
              gxwork[npt] = x[i-1];      gywork[npt] = y[i-1];
              npt++;
           }
           ComputeLogs(npt, OptionZ);
           if (gxworkl[npt-1] < rwxmin || gxworkl[npt-1] > rwxmax) {
              if (npt > 2) {
                 ComputeLogs(npt, OptionZ);
                 Smooth(npt,gxworkl,gyworkl,drawtype);
              }
              gxwork[0] = gxwork[npt-1]; gywork[0] = gywork[npt-1];
              npt=1;
              continue;
           }
        } //endfor (i=1;i<=nloop;i++)
        if (npt > 1) {
           ComputeLogs(npt, OptionZ);
           Smooth(npt,gxworkl,gyworkl,drawtype);
        }
     }
  }

//*-*-           Draw the graph with a '*' on every points

  if (OptionStar) {
     SetMarkerStyle(3);
     npt = 0;
     for (i=1;i<=npoints;i++) {
        if (y[i-1] >= minimum && y[i-1] <= maximum && x[i-1] >= uxmin  && x[i-1] <= uxmax) {
           gxwork[npt] = x[i-1];      gywork[npt] = y[i-1];
           npt++;
        }
        if (i == npoints) {
           ComputeLogs(npt, OptionZ);
	   
	   //Added to TGraph code
	   cout << "Color: " << fcolor[i-1] << endl;

	   if(fcolor)
	     {
	       SetMarkerColor(fcolor[i-1]);
	       cout << "set color to: " << fcolor[i-1] << endl;
	     }
           //////////////////////

	   if (OptionR)  gPad->PaintPolyMarker(npt,gyworkl,gxworkl);
           else          gPad->PaintPolyMarker(npt,gxworkl,gyworkl);
           npt = 0;
        }
     }
  }

//*-*-           Draw the graph with the current polymarker on
//*-*-           every points

  if (OptionMark) {
     npt = 0;
     
     Int_t* cwork = NULL;

     if(fcolor)
       cwork = new Int_t[npoints];

     for (i=1;i<=npoints;i++) {
        if (y[i-1] >= minimum && y[i-1] <= maximum && x[i-1] >= uxmin  && x[i-1] <= uxmax) {
           gxwork[npt] = x[i-1];      gywork[npt] = y[i-1];
	   if(fcolor)
	     {
	       cwork[npt] = fcolor[i-1];
	     }
           npt++;
        }
     }
     ComputeLogs(npt, OptionZ);
     
     //Added to TGraph code

     for(Int_t c = 0; c < npt ; c++)
       {
	
	 cout << "Color: " << cwork[c] << endl;

	 if(fcolor)
	   {
	     gVirtualX->SetMarkerColor(cwork[c]);
	     //SetMarkerStyle(2);
	     cout << "set color to: " << cwork[c] << endl;
	   }
           //////////////////////

	   if (OptionR) 
	     gPad->PaintPolyMarker(1,gyworkl+c,gxworkl+c);
	   else
	     gPad->PaintPolyMarker(1,gxworkl+c,gyworkl+c);	     
       }
     if(fcolor)
       delete cwork;
     npt = 0;

  }

//*-*-           Draw the graph as a bar chart

  if (OptionBar) {
     if (!OptionR) {
        barxmin = x[0];
        barxmax = x[0];
        for (i=1;i<npoints;i++) {
           if (x[i] < barxmin) barxmin = x[i];
           if (x[i] > barxmax) barxmax = x[i];
        }
        bdelta = (barxmax-barxmin)/Double_t(npoints);
     }
     else {
        barymin = y[0];
        barymax = y[0];
        for (i=1;i<npoints;i++) {
           if (y[i] < barymin) barymin = y[i];
           if (y[i] > barymax) barymax = y[i];
        }
        bdelta = (barymax-barymin)/Double_t(npoints);
     }
     dbar  = 0.5*bdelta*gStyle->GetBarWidth();
     if (!OptionR) {
        for (i=1;i<=npoints;i++) {
           xlow  = x[i-1] - dbar;
           xhigh = x[i-1] + dbar;
           yhigh = y[i-1];
           if (xlow  < uxmin) continue;
           if (xhigh > uxmax) continue;
           if (!OptionOne) ylow = TMath::Max((Double_t)0,gPad->GetUymin());
           else            ylow = gPad->GetUymin();
           gxwork[0] = xlow;
           gywork[0] = ylow;
           gxwork[1] = xhigh;
           gywork[1] = yhigh;
           ComputeLogs(2, OptionZ);
           if (gyworkl[0] < gPad->GetUymin()) gyworkl[0] = gPad->GetUymin();
           if (gyworkl[1] < gPad->GetUymin()) continue;
           if (gyworkl[1] > gPad->GetUymax()) gyworkl[1] = gPad->GetUymax();
           if (gyworkl[0] > gPad->GetUymax()) continue;

           gPad->PaintBox(gxworkl[0],gyworkl[0],gxworkl[1],gyworkl[1], "B");
        }
     }
     else {
        for (i=1;i<=npoints;i++) {
           xhigh = x[i-1];
           ylow  = y[i-1] - dbar;
           yhigh = y[i-1] + dbar;
           xlow     = TMath::Max((Double_t)0, gPad->GetUxmin());
           gxwork[0] = xlow;
           gywork[0] = ylow;
           gxwork[1] = xhigh;
           gywork[1] = yhigh;
           ComputeLogs(2, OptionZ);
           gPad->PaintBox(gxworkl[0],gyworkl[0],gxworkl[1],gyworkl[1], "B");
        }
     }
  }
   gPad->ResetBit(kClipFrame);

   delete [] gxwork;
   delete [] gywork;
   delete [] gxworkl;
   delete [] gyworkl;
}

//______________________________________________________________________________
ClassImp(THaGraph)
