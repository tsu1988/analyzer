#ifndef ROOT_THaEventTrack
#define ROOT_THaEventTrack

//////////////////////////////////////////////////////////////////////
//
// THaEventTrack
//
//////////////////////////////////////////////////////////////////////


#include "THaPhysicsModule.h"
#include "THaApparatus.h"
#include "TCanvas.h"
#include "TVector3.h"
#include "TGeometry.h"
#include "THaSpectrometer.h"
#include "THaCut.h"
#include "TCondition.h"
#include "TThread.h"
#include "TButton.h"
#include "THaETVDCGrDetail.h"

class THaCut;

class THaEventTrack : public THaPhysicsModule {

public:
  THaEventTrack(const char* name,  THaApparatus* app);
  virtual ~THaEventTrack();

  virtual EStatus 	Init( const TDatime& run_time );
  virtual Int_t		Process( const THaEvData& evdata);

  	  void		SetCentralAngle(TVector3 cangle);
	void            Next();
	void            Quit();
        void            Skip();
	void            Detail();
	void            Finish();
protected:
	
	THaSpectrometer* fSpectro;
  	THaApparatus* 	fApp;
  	TCanvas*	fCanvas;
	THaCut*		fTest;
	TButton*        fbutnext;
	TButton*        fbutquit;
	TButton*        fbutskip;
	TButton*        fbutVDC;
	TButton*        fbutFinish;
	
	Int_t		fFlag;
	Int_t           fFlags;
	Int_t		fCount;
	bool		fIsSetup;
	
  	TVector3	fCentralAngle;

  	Int_t		DrawDetector(const char* name,Float_t sx,Float_t sy,Float_t d);
	Int_t           DrawEvent(const THaEvData& evdata);
	void            InitGraphics();
	static void     ProcessThread(void* arg);
	

        void            AddButtons();
	void            DrawButtons();

	TList           fDetWindows;
  	TList		fGraphics;
 	TGeometry*	fGeom; 

	const THaEvData       *fcurevent;
	  
	enum EFlags {
	      kStop  = BIT(0),    // Wait for key press after every event
	      kCount = BIT(1),    // Run for fCount events
	      kQuiet = BIT(2)     // Run quietly (don't print variables)
	};

private:

  
//  THaEventTrack();
//  THaEventTrack( const THaEventTrack& );
//  THaEventTrack& operator=( const THaEventTrack& );

  ClassDef(THaEventTrack,0)  
};
inline void THaEventTrack::SetCentralAngle(TVector3 cangle)
{
	fCentralAngle = cangle;
}

#endif
