#ifndef _ROOT_THAGRAPH_H
#define _ROOT_THAGRAPH_H

#include "TF1.h"
#include "TGraph.h"
#include "TAttFill.h"

class THaGraph : public TGraph 
{

public:
	THaGraph(Int_t n, const Double_t* x, const Double_t* y, Int_t* c =NULL);
	virtual ~THaGraph();

	virtual void PaintGraph(Int_t npoints, const Double_t* x, const Double_t* y, Option_t* option = NULL);
	  
protected:

	Int_t* fcolor;	

ClassDef(THaGraph,0)	

};




#endif


