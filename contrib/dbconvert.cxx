// ---- VDC ----
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // load global VDC parameters
  static const char* const here = "ReadDatabase";
  const int LEN = 200;
  char buff[LEN];

  // Look for the section [<prefix>.global] in the file, e.g. [ R.global ]
  TString tag(fPrefix);
  Ssiz_t pos = tag.Index(".");
  if( pos != kNPOS )
    tag = tag(0,pos+1);
  else
    tag.Append(".");
  tag.Prepend("[");
  tag.Append("global]");

  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != 0) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section %s not found!", tag.Data() );
    fclose(file);
    return kInitError;
  }

  // We found the section, now read the data

  // read in some basic constants first
  //  fscanf(file, "%lf", &fSpacing);
  // fSpacing is now calculated from the actual z-positions in Init(),
  // so skip the first line after [ global ] completely:
  fgets(buff, LEN, file); // Skip rest of line

  // Read in the focal plane transfer elements
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  //
  // [ 2002-10-10 15:30:00 ]
  // comment line goes here
  // t 0 0 0  ...
  // y 0 0 0  ... etc.
  //
  // or
  //
  // [ config=highmom ]
  // comment line
  // t 0 0 0  ... etc.
  //
  if( (found = SeekDBdate( file, date )) == 0 && !fConfig.IsNull() &&
      (found = SeekDBconfig( file, fConfig.Data() )) == 0 ) {
    // Print warning if a requested (non-empty) config not found
    Warning( Here(here), "Requested configuration section \"%s\" not "
	     "found in database. Using default (first) section.",
	     fConfig.Data() );
  }

  // Second line after [ global ] or first line after a found tag.
  // After a found tag, it must be the comment line. If not found, then it
  // can be either the comment or a non-found tag before the comment...
  fgets(buff, LEN, file);  // Skip line
  if( !found && IsTag(buff) )
    // Skip one more line if this one was a non-found tag
    fgets(buff, LEN, file);

  fTMatrixElems.clear();
  fDMatrixElems.clear();
  fPMatrixElems.clear();
  fPTAMatrixElems.clear();
  fYMatrixElems.clear();
  fYTAMatrixElems.clear();
  fLMatrixElems.clear();

  fFPMatrixElems.clear();
  fFPMatrixElems.resize(3);

  typedef vector<string>::size_type vsiz_t;
  map<string,vsiz_t> power;
  power["t"] = 3;  // transport to focal-plane tensors
  power["y"] = 3;
  power["p"] = 3;
  power["D"] = 3;  // focal-plane to target tensors
  power["T"] = 3;
  power["Y"] = 3;
  power["YTA"] = 4;
  power["P"] = 3;
  power["PTA"] = 4;
  power["L"] = 4;  // pathlength from z=0 (target) to focal plane (meters)
  power["XF"] = 5; // forward: target to focal-plane (I think)
  power["TF"] = 5;
  power["PF"] = 5;
  power["YF"] = 5;

  map<string,vector<THaMatrixElement>*> matrix_map;
  matrix_map["t"] = &fFPMatrixElems;
  matrix_map["y"] = &fFPMatrixElems;
  matrix_map["p"] = &fFPMatrixElems;
  matrix_map["D"] = &fDMatrixElems;
  matrix_map["T"] = &fTMatrixElems;
  matrix_map["Y"] = &fYMatrixElems;
  matrix_map["YTA"] = &fYTAMatrixElems;
  matrix_map["P"] = &fPMatrixElems;
  matrix_map["PTA"] = &fPTAMatrixElems;
  matrix_map["L"] = &fLMatrixElems;

  map <string,int> fp_map;
  fp_map["t"] = 0;
  fp_map["y"] = 1;
  fp_map["p"] = 2;

  // Read in as many of the matrix elements as there are.
  // Read in line-by-line, so as to be able to handle tensors of
  // different orders.
  while( fgets(buff, LEN, file) ) {
    string line(buff);
    // Erase trailing newline
    if( line.size() > 0 && line[line.size()-1] == '\n' ) {
      buff[line.size()-1] = 0;
      line.erase(line.size()-1,1);
    }
    // Split the line into whitespace-separated fields
    vector<string> line_spl = Split(line);

    // Stop if the line does not start with a string referring to
    // a known type of matrix element. In particular, this will
    // stop on a subsequent timestamp or configuration tag starting with "["
    if(line_spl.empty())
      continue; //ignore empty lines
    const char* w = line_spl[0].c_str();
    vsiz_t npow = power[w];
    if( npow == 0 )
      break;

    // Looks like a good line, go parse it.
    THaMatrixElement ME;
    ME.pw.resize(npow);
    ME.iszero = true;  ME.order = 0;
    vsiz_t pos;
    for (pos=1; pos<=npow && pos<line_spl.size(); pos++) {
      ME.pw[pos-1] = atoi(line_spl[pos].c_str());
    }
    vsiz_t p_cnt;
    for ( p_cnt=0; pos<line_spl.size() && p_cnt<kPORDER && pos<=npow+kPORDER;
	  pos++,p_cnt++ ) {
      ME.poly[p_cnt] = atof(line_spl[pos].c_str());
      if (ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    if (p_cnt < 1) {
	Error(Here(here), "Could not read in Matrix Element %s%d%d%d!",
	      w, ME.pw[0], ME.pw[1], ME.pw[2]);
	Error(Here(here), "Line looks like: %s",line.c_str());
	fclose(file);
	return kInitError;
    }
    // Don't bother with all-zero matrix elements
    if( ME.iszero )
      continue;

    // Add this matrix element to the appropriate array
    vector<THaMatrixElement> *mat = matrix_map[w];
    if (mat) {
      // Special checks for focal plane matrix elements
      if( mat == &fFPMatrixElems ) {
	if( ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
	  THaMatrixElement& m = (*mat)[fp_map[w]];
	  if( m.order > 0 ) {
	    Warning(Here(here), "Duplicate definition of focal plane "
		    "matrix element: %s. Using first definition.", buff);
	  } else
	    m = ME;
	} else
	  Warning(Here(here), "Bad coefficients of focal plane matrix "
		  "element %s", buff);
      }
      else {
	// All other matrix elements are just appended to the respective array
	// but ensure that they are defined only once!
	bool match = false;
	for( vector<THaMatrixElement>::iterator it = mat->begin();
	     it != mat->end() && !(match = it->match(ME)); ++it ) {}
	if( match ) {
	  Warning(Here(here), "Duplicate definition of "
		  "matrix element: %s. Using first definition.", buff);
	} else
	  mat->push_back(ME);
      }
    }
    else if ( fDebug > 0 )
      Warning(Here(here), "Not storing matrix for: %s !", w);

  } //while(fgets)

  // Compute derived quantities and set some hardcoded parameters
  const Double_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  // Define the VDC coordinate axes in the "detector system". By definition,
  // the detector system is identical to the VDC origin in the Hall A HRS.
  DefineAxes(0.0*degrad);

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // figure out the track length from the origin to the s1 plane

  // since we take the VDC to be the origin of the coordinate
  // space, this is actually pretty simple
  const THaDetector* s1 = 0;
  if( GetApparatus() )
    s1 = GetApparatus()->GetDetector("s1");
  if(s1 == 0)
    fCentralDist = 0;
  else
    fCentralDist = s1->GetOrigin().Z();

  CalcMatrix(1.,fLMatrixElems); // tensor without explicit polynomial in x_fp

  // FIXME: Set geometry data (fOrigin). Currently fOrigin = (0,0,0).

  fIsInit = true;
  fclose(file);
