#ifndef ROOT_THaETDetail
#define ROOT_THaETDetail

///////////////////////////////////////////////////////////////////////
//
//
//  THaETDetail
//
//
///////////////////////////////////////////////////////////////////////


#include "THaPhysicsModule.h"

class THaEvData;


class THaETDetail :public TNamed
{
 public:

  THaETDetail();
  THaETDetail(const char* name, const char* title = NULL);
  virtual ~THaETDetail();
 
  virtual Int_t       Process(const THaEvData& evdata) = 0;
  virtual THaAnalysisObject::EStatus     Init() = 0;

 protected:

  

 private:

  ClassDef(THaETDetail,0);

};

#endif
