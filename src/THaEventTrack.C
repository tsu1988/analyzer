//*-- Author : 	Robert Stringer	17-Jun-03
//
//////////////////////////////////////////////////////
//
// THaEventTrack
//
// Physics Module to display single/multi track events 
// through the detector stack.
//
/////////////////////////////////////////////////////

#include "THaEventTrack.h"
#include "TVector3.h"
#include "THaSpectrometerDetector.h"
#include <math.h>
#include "TBox.h"
#include "THaTrack.h"
#include "TRotMatrix.h"
#include "TBRIK.h"
#include "TNode.h"
#include "TButton.h"


//----------------------------------------------------
THaEventTrack::THaEventTrack(const char* name, THaApparatus* app): THaPhysicsModule(name,"EventTrack")
{
  printf("In Const");
  if(!app || !app->InheritsFrom("THaSpectrometer")) //Do not allow null apparatus.
  {
    printf("Not a spec");
    Error( Here("THaEventTrack()"), "Apparatus %s is not a THaSpectrometer.",
	app->GetName());
    fApp=NULL;
  }
  else
  {
    fApp = app;
    SetCentralAngle(TVector3(1,1,1));
  }
}

//----------------------------------------------------
THaEventTrack::~THaEventTrack()
{
  // Destructor
	

 // Free Graphics
	
  TIter itr(&fGraphics);
  TObject *t;
  while(t = itr.Next())
  {
    printf("Freed.\n");
    delete t;
  }
}
//----------------------------------------------------
Int_t THaEventTrack::DrawEvent(const THaEvData& evdata )
{
  // do something ...
 
  //Test 
 /* 
    if( !fIsSetup ) 
    {
	fTest = new THaCut( "_Test", "" , "_Block" );
	// Expression error?
	if( !fTest || fTest->IsZombie()) 
	{
	  delete fTest; fTest = NULL;
        }
	fIsSetup = true;
    }
    bool good = true;
    if ( fTest && !fTest->EvalCut())
	    return 0;
                             
*/
	
  Int_t ntracks = fSpectro->GetNTracks();
  if( ntracks == 0 )
	  return 0;

//test-look at only multi-track events.
//  if( ntracks == 1 )
//	  return 0;
  
  TClonesArray* tracks = fSpectro->GetTracks();
  if (!tracks)
	  return 0;
  
  //Clear old hits

  TList* nodes = (fGeom->GetCurrentNode())->GetListOfNodes();
  TIter next(nodes);
  TNode* obj;
  
  while( obj = (TNode*) next())
  {  

//   cout << "Node:" << obj->GetName() << obj <<"\n";
    if(TString(obj->GetName()).Contains("HIT"))
    {
      fGeom->RecursiveRemove(obj);
    }
  }

   
  Int_t ndetectors = fApp->GetNumDets();

  for(Int_t i = 0; i < ntracks; i++)
  { 
    THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i));
    if(!theTrack || ! theTrack->HasTarget() )
    	continue;

    cout << "Num dets:" << ndetectors << endl;

    for(Int_t i=0 ;i < ndetectors;i++)
    {
      THaDetector* dobj =  fApp->GetDetector(i);
      Double_t t;

      cout << "Detector #" << i << ": "<<  dobj->ClassName() << endl;

      if(TString(dobj->ClassName()) == "THaScintillator" ||
	 TString(dobj->ClassName()) == "THaShower") 
      {
	cout << "Evdata Draw" << endl;
	dobj->Draw(fGeom,evdata);
      }
      else
      {      
        dobj->Draw(fGeom,theTrack,t);
	dobj->Draw(fGeom,evdata);
      }
    }
			  
  }

  fGeom->Draw();

  AddButtons();
  DrawButtons();


  fCanvas->Update();

  return 1;

}
//----------------------------------------------------
Int_t THaEventTrack::Process( const THaEvData& evdata )
{
  //Event loop.

  fFlag = 0;


  if(!DrawEvent(evdata))
    return 0;
 
  
 /*
  // Wait for user input
  cout << "RETURN: continue, H: run 100 events, R: run to end, F: finish quietly, Q: quit\n";
   char c;
   cin.clear();
   while( !cin.eof() && cin.get(c) && !strchr("\nqQhHrRfF",c));
 

  if( c != '\n' ) while( !cin.eof() && cin.get() != '\n');
    if( tolower(c) == 'q' )
      return kTerminate;
    else if( tolower(c) == 'h' ) 
    {
       fFlags |= kCount;
       fFlags &= ~kStop;
       fCount = 100;
    }
    else if( tolower(c) == 'r' )
    fFlags &= ~kStop;
  else if( tolower(c) == 'f' ) 
  {
    fFlags &= ~kStop;
    fFlags |= kQuiet;
  }
 */
     
 while(fFlag==0)
   {
     gSystem->ProcessEvents();
     gSystem->Sleep(10);
   }

  if(fFlag == kTerminate)
    return fFlag;                                                                        
  return 0;
}

//----------------------------------------------------
THaAnalysisObject::EStatus THaEventTrack::Init( const TDatime& run_time)
{
  // Initialize the module.

  cout << "Init EventTrack." << endl;

  if( THaPhysicsModule::Init( run_time ))
  {
    return fStatus;
  }

  if(!fApp)
  {
	  return kInitError;
  }

  fCanvas = NULL;


  cout << "Creating thread." << endl;

  //fThread = new TThread("Thread",(void(*)(void *)) &ProcessThread, (void*) this);
  
  //fThread->Run();


  InitGraphics();
  //AddButtons();

  return kOK;
}

void THaEventTrack::ProcessThread(void* arg)
{

  THaEventTrack* newthis = (THaEventTrack*) arg;

    cout << "Thread Started." << endl;

    while (true)
      {
	gSystem->ProcessEvents();
	gSystem->Sleep(10);
      }

}

void THaEventTrack::InitGraphics()
{


  //Initialize the Canvas

  TRotMatrix* zero = NULL;
  fCanvas = new TCanvas("EventTrack");
  fCanvas->SetEditable();

  //Fix this: don't create matrices in event class.

  fGeom = new TGeometry("EventTrack","EventTrack");
  fGeom->GetListOfMatrices()->Add(new TRotMatrix("XZ","XZ",45,0,90,90,-45,0));
  zero = new TRotMatrix("ZERO","ZERO",90,0,90,90,0,0);
  fGeom->GetListOfMatrices()->Add(zero);

  fGeom->SetMatrix(zero);
  new TBRIK("DUM","DUM","void",0,0,0);
  fGeom->Node("DUMMY","Center","DUM" ,0,0,0);
  
  // Get the spectrometer

  fSpectro = static_cast<THaSpectrometer*>
	  ( FindModule( fApp->GetName(), "THaSpectrometer" ));
  
  if(!fSpectro )
	  return;

  //Find the included detectors.
  
  
  Int_t ndetectors = fApp->GetNumDets();

  TString buf(3);
  for(Int_t i= ndetectors-1 ;i >= 0;i--)
  {
    THaDetector* dobj =  fApp->GetDetector(i);
  
    buf="";
    buf+=i+1;

    dobj->Draw(fGeom,buf);
       
  }
 
  // DrawButtons();



 }
//----------------------------------------------------
Int_t THaEventTrack::DrawDetector(const char* name, Float_t sx, Float_t sy, Float_t d)
{
  //Draws the detectors on the canvas.

  Double_t x1 = (Double_t) d * fCentralAngle.CosTheta() - .5*sx;
  Double_t y1 = (Double_t) d * sin(fCentralAngle.Theta()) - .5*sy;
  Double_t x2 = (Double_t) d * fCentralAngle.CosTheta() + .5*sx;
  Double_t y2 = (Double_t) d * sin(fCentralAngle.Theta()) + .5*sy;
  	
  printf("%s: cos-angle = %f, dist = %f",name,fCentralAngle.CosTheta(),d);
  TBox* b = new TBox(x1/4,y1/4,x2/4,y2/4);

  fGraphics.Add(b);
  
  b->SetFillColor(0);
  b->SetFillStyle(0);
  b->Draw();

  return kOK;
}

//----------------------------------------------------
void THaEventTrack::AddButtons()
{

   fbutnext = new TButton("Next","evttrk->Next()",.1,.9,.3,.98);

   fbutskip = new TButton("Skip 100","evttrk->Skip()",.35,.9,.55,.98);

   fbutquit = new TButton("Quit","evttrk->Quit()",.6,.9,.8,.98);

   
}
void THaEventTrack::DrawButtons()
{

  fbutnext->Draw();
  fbutskip->Draw();
  fbutquit->Draw();

}


void THaEventTrack::Next()
{
  
  //fmsgcond->Signal();
  fFlag = 1;

}
void THaEventTrack::Quit()
{

  //fexitcond->Signal();
  fFlag = kTerminate;

}
void THaEventTrack::Skip()
{

  //fmsgcond->Signal();
  fCount = 100;
  fFlag = 1;

}

//----------------------------------------------------

ClassImp(THaEventTrack)
