//*-- Author :    Ole Hansen   03-Apr-13

//////////////////////////////////////////////////////////////////////////
//
// THaFileDB
//
// Backend for Hall A-style database files, stored in time-stamped
// directories.
//
//////////////////////////////////////////////////////////////////////////

#include "THaFileDB.h"
#include "THaGlobals.h"
#include "THaTextvars.h"
#include "TError.h"
#include <unistd.h>
#include <errno.h>
#include <cassert>
#include <cstring>

using namespace std;

typedef string::size_type ssiz_t;
typedef vector<string>::iterator vsiter_t;

//_____________________________________________________________________________
THaFileDB::THaFileDB( const char* DB_DIR ) : THaDB(DB_DIR)
{
  // Constructor
}

//_____________________________________________________________________________
THaFileDB::~THaFileDB()
{
  // Destructor

  Close();
}

//_____________________________________________________________________________
Int_t THaFileDB::Open() 
{
  // Open the database connection. Currently does nothing.
  // Eventually will read all database files under DB_DIR etc.

  return THaDB::Open();
}

//_____________________________________________________________________________
Int_t THaFileDB::Close() 
{
  // Close the database connection. Frees up memory used by internal containers.

  fKeys.clear();
  fValues.clear();
  fTimeRanges.clear();
  fVisitedFiles.clear();

  return THaDB::Close();
}

//_____________________________________________________________________________
Int_t THaFileDB::LookupKey( const char* key, string& text, time_t timet )
{
  // Look up the value for the given key and time as a string.
  // Returns 0 on success, 1 if key not found, <0 if unexpected error.

  typedef ValueMap_t::iterator valiter_t;
  typedef pair<valiter_t,valiter_t> value_range_t;

  if( !key || !*key ) return 1;

  // Map the key string to its ID
  StringMap_t::iterator ik = fKeys.find( string(key) );
  if( ik == fKeys.end() )  // No such key
    return 1;

  // Find the value corresponding to this key ID
  value_range_t range = fValues.equal_range( ik->second );
  assert( range.first != fValues.end() );  // Value must exist

  // Check if any of the value's time ranges matches the requested time
  for( valiter_t it = range.first; it != range.second; ++it ) {
    const Value_t& theValue = (*it).second;
    TimeRangeMap_t::iterator ir = fTimeRanges.find( theValue.range_id );
    assert( ir != fTimeRanges.end() );
    // The time ranges are unique by construction, so we only need to check
    // for a single match
    const TimeRange_t& theRange = (*ir).second;
    if( theRange.start_time <= timet && timet < theRange.end_time ) {
      text = theValue.value;
      return 0;
    }
  }
  return 1;
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadValue( const char* key, string& text, const TDatime& date )
{
  // Retrieve the value for the given key as a string. Converts the given date
  // to time_t, using the current locale, then calls LookupKey.
  // The TDatime object must be given in the local time of the current machine.

  return LookupKey( key, text, date.Convert() );
}

//_____________________________________________________________________________
static Int_t IsDBdate( const string& line, TDatime& date, bool warn = true )
{
  // Check if 'line' contains a valid database time stamp. If so, 
  // parse the line, set 'date' to the extracted time stamp, and return 1.
  // Else return 0;
  // Time stamps must be in SQL format: [ yyyy-mm-dd hh:mi:ss ]

  ssiz_t lbrk = line.find('[');
  if( lbrk == string::npos || lbrk >= line.size()-12 ) return 0;
  ssiz_t rbrk = line.find(']',lbrk);
  if( rbrk == string::npos || rbrk <= lbrk+11 ) return 0;
  Int_t yy, mm, dd, hh, mi, ss;
  if( sscanf( line.substr(lbrk+1,rbrk-lbrk-1).c_str(), "%d-%d-%d %d:%d:%d",
	      &yy, &mm, &dd, &hh, &mi, &ss) != 6
      || yy < 1995 || mm < 1 || mm > 12 || dd < 1 || dd > 31
      || hh < 0 || hh > 23 || mi < 0 || mi > 59 || ss < 0 || ss > 59 ) {
    if( warn )
      ::Warning("THaFileDB::IsDBdate()", "Invalid date tag %s", line.c_str());
    return 0;
  }
  date.Set(yy, mm, dd, hh, mi, ss);
  return 1;
}

//_____________________________________________________________________________
static Int_t ParseDBline( const string& line, string& key, string& text )
{
  // Check if 'line' is of the form "key = value". If so, trim both LHS and RHS
  // and put the results into 'key' and 'text', respectively.
  // - If there is no '=', then return 0.
  // - If the LHS is empty, return -1
  // - If parsing successful return +1.
  // 'key' and 'text' are not changed unless parsing is successful.
  //
  // Note: By construction in ReadDBline, 'line' is not empty, any comments
  // starting with '#' have been removed, and trailing whitespace has been
  // trimmed. Also, all tabs have been converted to spaces.

  // Search for "="
  register const char* ln = line.c_str();
  const char* eq = strchr(ln, '=');
  if( !eq ) return 0;
  // Extract the key
  while( *ln == ' ' ) ++ln;
  assert( ln <= eq );
  if( ln == eq ) return -1;
  register const char* p = eq-1;
  assert( p >= ln );
  while( *p == ' ' ) --p; // find_last_not_of(" ")
  key.assign( ln, p-ln+1 );
  // Extract the value, trimming leading whitespace.
  ln = eq+1;
  assert( !*ln || *(ln+strlen(ln)-1) != ' ' ); // Trailing space already trimmed
  while( *ln == ' ' ) ++ln;
  text = ln;
  return 1;
}

//_____________________________________________________________________________
static Int_t ReadDBline( FILE* file, char* buf, size_t bufsiz, string& line )
{
  // Get a text line from the database file 'file'. Ignore all comments
  // (anything after a #). Trim trailing whitespace. Concatenate continuation
  // lines (ending with \).
  // Only returns if a non-empty line was found, or on EOF.

  line.clear();

  char *r = buf;
  while( line.empty() && r ) {
    bool continued = false, unfinished = true, do_trim = false;
    // Must use C-style I/O because of the legacy FILE*
    while( unfinished && (r = fgets(buf, bufsiz, file)) ) {
      // Search for an end-of-data character
      char *c = strpbrk(buf, "\n#\\");
      if( !c ) c = buf+strlen(buf);
      int t = *c;
      *c = 0;
      // Convert tabs to spaces
      register char *p = buf;
      while( (p = strchr(p,'\t')) ) *(p++) = ' ';
      // Locate last non-blank character of the data
      char* lastc = 0;
      if( *buf ) {
	p = c;
	while( --p != buf && *p == ' ' );
	if( *p != ' ' ) lastc = p;
      }
      // Ensure that the whole line was read after comments and continuations
      bool read_more = ( t != '\n' && t != 0 && strchr(c+1,'\n') == 0 );

      // Continue reading if there is a continuation, the line wasn't fully
      // read yet, or the previous read was continued and now we have a pure 
      // comment line in the middle of a continuation
      bool only_comment = (t == '#' && !lastc);
      continued = ( t == '\\' || (only_comment && continued) );
      unfinished = (continued || !t );

      // Append current line's data to the output string
      if( !only_comment && (unfinished || lastc) ) {
	size_t len;
	if( unfinished ) {
	  do_trim = true;  // May have trailing blanks
	  len = c-buf;
	} else {
	  assert( lastc < c );
	  *(++lastc) = 0;
	  len = lastc-buf;
	}
	line.append(buf,len);
      }
      if( read_more ) do {
	  r = fgets(buf, bufsiz, file);
	} while( r && *r && buf[strlen(buf)-1] != '\n' );
    }
    if( line.empty() )
      continue;

    // Trim trailing whitespace
    if( do_trim ) {
      ssiz_t pos = line.find_last_not_of(" ");
      if( pos == string::npos )
	line.clear();
      else if( pos+1 != line.length() )
	line.erase(pos+1);
    }
  }
  // Don't report EOF yet if the last line wasn't empty
  if( r == 0 && !line.empty() )
    r = buf; // this is ok, the actual value of r will not be used

  return (r == 0) ? EOF : '\n';
}

//_____________________________________________________________________________
Int_t THaFileDB::LoadFile( FILE* file, const TDatime& date )
{
  // Load all database keys from the given file, unless already read.
  // The keys are assumed to be valid for the given date. 
  // No prefix is prepended to any keys found.

  static const char* const here = "THaFileDB::LoadFile";

  if( !file ) return -1;

  int fd = fileno(file);
#if defined(R__SEEK64)
  struct stat64 sbuf;
  if( fstat64(fd, &sbuf) != 0 ) {
#else
  stuct stat sbuf;
  if( fstat(fd, &sbuf) != 0 ) {
#endif
    perror("THaFileDB::LoadFile");
    return -1;
  }

  VisitedFile_t file_id( sbuf, date.Convert() );

  if( fVisitedFiles.find( file_id ) != fVisitedFiles.end() ) {
    // File already loaded: nothing to do
    return 0;
  }

  errno = 0;
  fErrtxt.Clear();
  rewind(file);

  static const size_t bufsiz = 128;
  char buf[bufsiz];
  string dbline, key, text;
  vector<string> lines;
  TDatime keydate(date);
  while( ReadDBline(file, buf, bufsiz, dbline) != EOF ) {
    if( dbline.empty() ) continue;
    // Replace text variables in this database line, if any. Multi-valued
    // variables are supported here, although they are only sensible on the LHS
    lines.assign( 1, dbline );
    gHaTextvars->Substitute( lines );
    for( vsiter_t it = lines.begin(); it != lines.end(); ++it ) {
      string& line = *it;
      Int_t status;
      if( (status = ParseDBline( line, key, text )) != 0 ) {
	if( status > 0 ) {
	  // Found a key
	  
	} else {
	  // Empty LHS
	  Warning( here, "Missing key in database line \"%s\"... Ignored.",
		   line.substr(0,47).c_str() );
	}
      } else if( IsDBdate( line, keydate ) != 0 )  {
	// Found a date
	
      } else {
	// No '=' and also no '[...]' -> unrecognized format
	Warning( here, "Unrecognized database line \"%s\"... Ignored.",
		 line.substr(0,47).c_str() );
      }
    }
  }

  if( errno ) {
    perror( "THaFile::LoadDBvalue" );
    return -1;
  }



  pair<FileSet_t::iterator,bool> ins = fVisitedFiles.insert( file_id );
  assert( ins.second );

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaFileDB)
