#ifndef ROOT_THaScintillator
#define ROOT_THaScintillator

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"

class THaScintillator : public THaNonTrackingDetector {

public:
  THaScintillator( const char* name, const char* description = "",
		   THaApparatus* a = NULL );
  virtual ~THaScintillator();

  virtual Int_t      Decode( const THaEvData& );
  virtual EStatus    Init( const TDatime& run_time );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
  virtual Double_t   CalcY(Int_t paddle);
  virtual Int_t      PaddlesHit(Int_t* hits);
  bool               MarkPaddle(TGeometry* geom, const char* detname, Int_t padnum,const char* color);
  virtual void       DrawPaddle(TGeometry* geom, const char* shapename,Int_t padnum);
  virtual void	     Draw(TGeometry* geom, const Option_t* opt = NULL);
  virtual void	     Draw(TGeometry* geom, const THaEvData& evdata, const Option_t* opt = NULL);
  virtual void       Draw(const Option_t* opt = NULL){};
  virtual void       DrawY(TGeometry* geom, Int_t paddle, Double_t y);

protected:

  // Mapping
  UShort_t*  fFirstChan;  // Beginning channels for each detmap module

  // Calibration
  Float_t*   fLOff;       // [fNelem] TDC offsets for left paddles
  Float_t*   fROff;       // [fNelem] TDC offsets for right paddles
  Float_t*   fLPed;       // [fNelem] ADC pedestals for left paddles
  Float_t*   fRPed;       // [fNelem] ADC pedestals for right paddles
  Float_t*   fLGain;      // [fNelem] ADC gains for left paddles
  Float_t*   fRGain;      // [fNelem] ADC gains for right paddles
  Float_t    fC;          // Velocity of light in medium
  Float_t    fTDCScale;   // ns to channel
  Float_t    fStop;       // Stop Trigger?
  
  
  // Per-event data
  Int_t      fLTNhit;     // Number of Left paddles TDC times
  Float_t*   fLT;         // [fNelem] Array of Left paddles TDC times
  Float_t*   fLT_c;       // [fNelem] Array of Left paddles corrected TDC times
  Int_t      fRTNhit;     // Number of Right paddles TDC times
  Float_t*   fRT;         // [fNelem] Array of Right paddles TDC times
  Float_t*   fRT_c;       // [fNelem] Array of Right paddles corrected TDC times
  Int_t      fLANhit;     // Number of Left paddles ADC amplitudes
  Float_t*   fLA;         // [fNelem] Array of Left paddles ADC amplitudes
  Float_t*   fLA_p;       // [fNelem] Array of Left paddles ADC minus ped values
  Float_t*   fLA_c;       // [fNelem] Array of Left paddles corrected ADC ampl-s
  Int_t      fRANhit;     // Number of Right paddles ADC amplitudes
  Float_t*   fRA;         // [fNelem] Array of Right paddles ADC amplitudes
  Float_t*   fRA_p;       // [fNelem] Array of Right paddles ADC minus ped values
  Float_t*   fRA_c;       // [fNelem] Array of Right paddles corrected ADC ampl-s
  Float_t    fTRX;        // x position of track cross point (cm)
  Float_t    fTRY;        // y position of track cross point (cm)

  TList      fPadObjs;    // List of Graphic objects

  // Useful derived quantities
  double tan_angle, sin_angle, cos_angle;
  
  static const char NDEST = 2;
  struct DataDest {
    Int_t*    nthit;
    Int_t*    nahit;
    Float_t*  tdc;
    Float_t*  tdc_c;
    Float_t*  adc;
    Float_t*  adc_p;
    Float_t*  adc_c;
    Float_t*  offset;
    Float_t*  ped;
    Float_t*  gain;
  } fDataDest[NDEST];     // Lookup table for decoder

  void           ClearEvent();
  void           DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode );

  ClassDef(THaScintillator,0)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
