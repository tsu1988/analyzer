//*-- Author : 	Robert Stringer	16-Jul-03
//
//////////////////////////////////////////////////////
//
// THaETDetail
//
// Abstract base class for Detector detail views in THaEvent Track. 
// 
//
/////////////////////////////////////////////////////

#include "THaETDetail.h"

//-------------------------------------------------------------------------

THaETDetail::THaETDetail()
{

}
//-------------------------------------------------------------------------

THaETDetail::THaETDetail(const char* name, const char* title) : TNamed(name,title)
{

}

//-------------------------------------------------------------------------

THaETDetail::~THaETDetail()
{

}



ClassImp(THaETDetail)
