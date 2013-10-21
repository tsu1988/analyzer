//TODO
class VDCeff : public THaPhysicsModule {
public:

  virtual Int_t   End(THaRunBase* r=0);
  virtual void    WriteHist(); 

  Int_t  cnt1;
  void VdcEff();

  static Int_t fgVdcEffFirst; //If >0, initialize VdcEff() on next call
};

