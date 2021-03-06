//o-----------------------------------------------------------------------------------------------o
//| cConsole.cpp
//o-----------------------------------------------------------------------------------------------o
//| In essence, CConsole is our nice generic class for outputting display.
//| As long as the interface is kept the same, we can display whatever we wish
//| too. We store a set of coordinates being the window corner and size, for
//| systems with windowing support, which describes the window we are in.
//o-----------------------------------------------------------------------------------------------o
//| Version History
//| 1.7		Added comments (29. October 2001)
//| 2.0		Added filter masking (22nd January 2002)
//o-----------------------------------------------------------------------------------------------o

// Header
#include "uox3.h"
#include "cConsole.h"
#include "cThreadQueue.h"
#include "ObjectFactory.h"
#include "network.h"
#include "ssection.h"
#include "CJSMapping.h"
#include "teffect.h"
#include "cMagic.h"
#include "magic.h"
#include "regions.h"
#include "commands.h"
#include "cServerDefinitions.h"
#include "cHTMLSystem.h"
#include "cRaces.h"
#include "cGuild.h"
#include "cScript.h"
#include "StringUtility.hpp"

#if UOX_PLATFORM != PLATFORM_WIN32
#include <stdio.h>

#include <sys/ioctl.h>
typedef void *HANDLE;
#else
#include <process.h>
#include <conio.h>
#endif


CConsole						Console;	// no *, else we can't overload <<
CEndL							myendl;

const UI08 NORMALMODE			= 0;
const UI08 WARNINGMODE			= 1;
const UI08 ERRORMODE			= 2;
//const UI08 COLOURMODE			= 3;

bool cluox_io					= false;   // is cluox-IO enabled?
bool cluox_nopipe_fill			= false;   // the stdin-pipe is known to be none-empty, no need to fill it.
HANDLE cluox_stdin_writeback	= 0; // the write-end of the stdin-pipe


// Forward function declarations

void		endmessage( SI32 x );
void		LoadCustomTitle( void );
void		LoadSkills( void );
void		LoadSpawnRegions( void );
void		LoadRegions( void );
void		LoadTeleportLocations( void );
void		LoadCreatures( void );
void		LoadPlaces( void );
void		UnloadRegions( void );
void		UnloadSpawnRegions( void );
void		LoadTeleportLocations( void );


//o-----------------------------------------------------------------------------------------------o
//|	Function	-	CConsole()
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Class Constructor and deconstructor
//o-----------------------------------------------------------------------------------------------o
CConsole::CConsole() : width( 80 ), height( 25 ),
#if UOX_PLATFORM == PLATFORM_WIN32
currentMode( NORMALMODE ), previousColour( CNORMAL ), logEcho( false )
#else
currentMode( NORMALMODE ), previousColour( CNORMAL ), logEcho( false ), forceNL( false )

#endif
{
#if UOX_PLATFORM != PLATFORM_WIN32
	tcgetattr (0, &resetio);
#endif
}

CConsole::~CConsole()
{
#if UOX_PLATFORM != PLATFORM_WIN32
	setvbuf(stdout, NULL, _IONBF, 0);
	tcsetattr(0, TCSANOW, &resetio);
#endif

}


//o-----------------------------------------------------------------------------------------------o
//| Function	-	<< Overrriding
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Let you use << and >>
//o-----------------------------------------------------------------------------------------------o
CConsole& CConsole::operator<<( const SI08 *outPut )
{
	StartOfLineCheck();
	(*this) << (const char *)outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const char *outPut )
{
	StartOfLineCheck();
#if UOX_PLATFORM == PLATFORM_WIN32
	CONSOLE_SCREEN_BUFFER_INFO ScrBuffInfo;
	GetConsoleScreenBufferInfo( hco, &ScrBuffInfo );
	std::string toDisplay = outPut;
	if( ScrBuffInfo.dwCursorPosition.X + toDisplay.length() > ScrBuffInfo.dwSize.X )
	{
		bool wrapDone = false;
		while( !wrapDone )
		{
			GetConsoleScreenBufferInfo( hco, &ScrBuffInfo );
			SI32 diff = ScrBuffInfo.dwSize.X - ScrBuffInfo.dwCursorPosition.X - 1;
			if( diff > toDisplay.length() )
			{
				std::cout << toDisplay;
				wrapDone = true;
			}
			else
			{
				std::cout << toDisplay.substr( 0, diff );
				(*this) << myendl;
				StartOfLineCheck();
				toDisplay = toDisplay.substr( diff );
			}
		}
	}
	else
#endif
	{
		std::cout << outPut;
	}
	return (*this);
}
CConsole& CConsole::operator<<( const UI08 *outPut )
{
	StartOfLineCheck();
	(*this) << (const char *)outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const SI08 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const UI08 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const SI16 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const UI16 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const SI32 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const UI32 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const SI64 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return ( *this );
}
CConsole& CConsole::operator<<( const UI64 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const CBaseObject *outPut )
{
	StartOfLineCheck();
	(*this) << outPut->GetSerial();
	return (*this);
}
CConsole& CConsole::operator<<( const std::string &outPut )
{
	StartOfLineCheck();
	std::cout << outPut.c_str();
	return (*this);
}
/*CConsole& CConsole::operator<<( const std::ostream& outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}*/
CConsole& CConsole::operator<<( CBaseObject *outPut )
{
	StartOfLineCheck();
	(*this) << outPut->GetSerial();
	return (*this);
}


CConsole& CConsole::operator<<( CEndL& myObj )
{
	if( curLeft == 0 )
		PrintStartOfLine();
	std::cout << std::endl;
	curLeft = 0;
	++curTop;
	return (*this);
}
CConsole& CConsole::operator<<( const R32 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}
CConsole& CConsole::operator<<( const R64 &outPut )
{
	StartOfLineCheck();
	std::cout << outPut;
	return (*this);
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void print(const std::string& msg) {
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Prints the console with the "|"
//o-----------------------------------------------------------------------------------------------o
void CConsole::print(const std::string& msg) {
	StartOfLineCheck() ;
	std::cout << msg ;
	if ((msg.size() > 0) && (msg[msg.size()-1]=='\n')) {
		curLeft = 0 ;
	}
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void log( const std::string& msg, const std::string& filename )
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Writes to the logfile
//o-----------------------------------------------------------------------------------------------o
void CConsole::log( const std::string& msg, const std::string& filename )
{
	if( !cwmWorldState->ServerData()->ServerConsoleLogStatus() )
		return;

	std::ofstream toWrite;
	std::string realFileName;	// 022602: in windows a path can be max 512 chars, this at 128 coud potentially cause crashes if the path is longer than 128 chars
	if( cwmWorldState != NULL )
		realFileName = cwmWorldState->ServerData()->Directory( CSDDP_LOGS ) + filename;
	else
		realFileName = filename;

	char timeStr[128];
	RealTime( timeStr );

	toWrite.open( realFileName.c_str(), std::ios::out | std::ios::app );
	if( toWrite.is_open() )
		toWrite << "[" << timeStr << "] " << msg << std::endl;
	toWrite.close();
	if( LogEcho() )
	{
		print( format( "%s%s\n", timeStr, msg.c_str()));
	}
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void log( const std::string& msg)
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Log to the console.log file
//o-----------------------------------------------------------------------------------------------o
void CConsole::log( const std::string& msg)
{
	if( !cwmWorldState->ServerData()->ServerConsoleLogStatus() )
		return;

	log( msg, "console.log" );
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void error( const std::string& msg )
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Writes to the error log and the console.
//o-----------------------------------------------------------------------------------------------o
void CConsole::error( const std::string& msg )
{
	UI08 oldMode = CurrentMode();
	CurrentMode( ERRORMODE );
	log( msg, "error.log" );
	if( curLeft != 0 )
		(*this) << myendl;
	TurnRed();
	(*this) << "ERROR: " << (const char*)&msg[0] << myendl;
	TurnNormal();
	CurrentMode( oldMode );
}


//o-----------------------------------------------------------------------------------------------o
//| Function	-	void PrintSectionBegin( void )
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Prints a section seperator
//o-----------------------------------------------------------------------------------------------o
void CConsole::PrintSectionBegin( void )
{
	TurnBrightWhite();
	std::cout << "o";
	for( SI32 j = 1; j < width - 1; ++j )
		std::cout << "-";
	std::cout << "o";
	curLeft = 0;
	curTop = 0;
	TurnNormal();
#if UOX_PLATFORM != PLATFORM_WIN32
	if( forceNL )
	{
		(*this) << myendl;
	}
#endif
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void Start( char *temp )
//| Date		-	24th December, 2001
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Migration of constart function to here
//o-----------------------------------------------------------------------------------------------o
void CConsole::Start( const std::string& temp )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	hco		= GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( hco, &csbi );
	width	= csbi.dwSize.X;
	height	= csbi.dwSize.Y;
	SetConsoleTitle( temp.c_str() );
#else
	// TODO: unix console handling should really be replaced by (n)curses or
	// something

	if (isatty (0))
	{
		struct termios tio;
		struct winsize winsz;

		// switch to raw mode
		tcgetattr (0, &tio);

		tio.c_lflag &= ~ICANON & ~ECHO;

		tcsetattr (0, TCSAFLUSH, &tio); //ignore errors

		// get window size
		ioctl (0, TIOCGWINSZ, &winsz);

		width = winsz.ws_col;
		height = winsz.ws_row;

		// disable stdout buffering
		setvbuf (stdout, NULL, _IONBF, 0);
	}
	else
	{
		// produce readable log
		forceNL = true;
	}
#endif
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void TurnYellow( void )
//| Date		-	24th December, 2001
///o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Turns the text yellow
//o-----------------------------------------------------------------------------------------------o
void CConsole::TurnYellow( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	SetConsoleTextAttribute( hco, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY );
#else
	std::cout << "\033[1;33m";
#endif
	previousColour = CYELLOW;
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void TurnRed( void )
//| Date		-	24th December, 2001
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Turns the text red
//o-----------------------------------------------------------------------------------------------o
void CConsole::TurnRed( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	SetConsoleTextAttribute( hco, FOREGROUND_RED | FOREGROUND_INTENSITY );
#else
	std::cout << "\033[1;31m";
#endif
	previousColour = CRED;
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void TurnGreen( void )
//| Date		-	24th December, 2001
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Turns the text green
//o-----------------------------------------------------------------------------------------------o
void CConsole::TurnGreen( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	SetConsoleTextAttribute( hco, FOREGROUND_GREEN | FOREGROUND_INTENSITY );
#else
	std::cout << "\033[1;32m";
#endif
	previousColour = CGREEN;
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void TurnBlue( void )
//| Date		-	24th December, 2001
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Turns the text blue
//o-----------------------------------------------------------------------------------------------o
void CConsole::TurnBlue( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	SetConsoleTextAttribute( hco, FOREGROUND_BLUE | FOREGROUND_INTENSITY );
#else
	std::cout << "\033[1;34m";
#endif
	previousColour = CBLUE;
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void TurnNormal( void )
//| Date		-	24th December, 2001
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Returns text to normal white
//o-----------------------------------------------------------------------------------------------o
void CConsole::TurnNormal( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	SetConsoleTextAttribute( hco, FOREGROUND_BLUE  | FOREGROUND_RED | FOREGROUND_GREEN );
#else
	std::cout << "\033[0;37m";
#endif
	previousColour = CNORMAL;
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void TurnBrightWhite( void )
//| Date		-	22nd January, 2002
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Returns text to bright white
//o-----------------------------------------------------------------------------------------------o
void CConsole::TurnBrightWhite( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	SetConsoleTextAttribute( hco, FOREGROUND_BLUE  | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY );
#else
	std::cout << "\033[1;37m";
#endif
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void PrintDone( void )
//|	Date		-	24th December, 2001
//|	Changes		-	7th February, 2003 - Made it use PrintSpecial
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Prints colored "[done]" message
//o-----------------------------------------------------------------------------------------------o
void CConsole::PrintDone( void )
{
	PrintSpecial( CGREEN, "done" );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void PrintFailed( void )
//|	Date		-	24th December, 2001
//|	Changes		-	7th February, 2003 - Made it use PrintSpecial
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Prints colored "[failed]" message
//o-----------------------------------------------------------------------------------------------o
void CConsole::PrintFailed( void )
{
	PrintSpecial( CRED, "Failed" );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void PrintPassed( void )
//|	Date		-	17th February, 2002
//|	Changes		-	7th February, 2003 - Made it use PrintSpecial
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Prints colored "[skipped]" message
//o-----------------------------------------------------------------------------------------------o
void CConsole::PrintPassed( void )
{
	PrintSpecial( CYELLOW, "Skipped" );
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void ClearScreen( void )
//| Date		-	24th December, 2001
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Clears the screen
//o-----------------------------------------------------------------------------------------------o
void CConsole::ClearScreen( void )
{
#if UOX_PLATFORM == PLATFORM_WIN32
	unsigned long y;
	COORD xy;

	xy.X = 0;
	xy.Y = 0;
	FillConsoleOutputCharacter( hco, ' ', width * height, xy, &y );
	SetConsoleCursorPosition( hco, xy );
#else
	std::cout << "\033[H" ;
#endif
}

void CConsole::PrintBasedOnVal( bool value )
{
	if( value )
		PrintDone();
	else
		PrintFailed();
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void warning( const std::string& msg )
//|	Date		-	21st January, 2002
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Writes to the warning log and the console.
//o-----------------------------------------------------------------------------------------------o
void CConsole::warning( const std::string& msg )
{
	UI08 oldMode = CurrentMode();
	CurrentMode( WARNINGMODE );
	log( msg, "warning.log" );
	if( curLeft != 0 )
		(*this) << myendl;
	TurnBlue();
	(*this) << "WARNING: " << (const char*)&msg[0] << myendl;
	TurnNormal();
	CurrentMode( oldMode );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	UI08 CurrentMode( void ) const
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets/Sets console's output mode
//o-----------------------------------------------------------------------------------------------o
UI08 CConsole::CurrentMode( void ) const
{
	return currentMode;
}
void CConsole::CurrentMode( UI08 value )
{
	currentMode = value;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void PrintStartOfLine( void )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Set colour of console text at start of a new line
//o-----------------------------------------------------------------------------------------------o
void CConsole::PrintStartOfLine( void )
{
	TurnBrightWhite();
	std::cout << "| ";
	switch( previousColour )
	{
		case CBLUE:		TurnBlue();		break;
		case CRED:		TurnRed();		break;
		case CGREEN:	TurnGreen();	break;
		case CYELLOW:	TurnYellow();	break;
		case CBWHITE:					break;	// current colour
		case CNORMAL:
		default:		TurnNormal();	break;
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void MoveTo( SI32 x, SI32 y )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Set console cursor position
//o-----------------------------------------------------------------------------------------------o
#if UOX_PLATFORM == PLATFORM_WIN32
void CConsole::MoveTo( SI32 x, SI32 y )
{
	COORD Pos;
	if( y == -1 )
	{
		CONSOLE_SCREEN_BUFFER_INFO ScrBuffInfo;

		GetConsoleScreenBufferInfo( hco, &ScrBuffInfo );
		Pos.Y = ScrBuffInfo.dwCursorPosition.Y;
		Pos.X = (UI16)x;
		SetConsoleCursorPosition( hco, Pos );
	}
	else
	{
		Pos.X = (UI16)x;
		Pos.Y = (UI16)y;
		SetConsoleCursorPosition( hco, Pos );
	}
}
#else
void CConsole::MoveTo( SI32 x, SI32 y )
{
	std::cout << "\033[255D";
	std::cout << "\033[" << x << "C";
}
#endif

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	bool LogEcho( void )
//|	Date		-	2/03/2003
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Checks whether console should echo log messages
//o-----------------------------------------------------------------------------------------------o
bool CConsole::LogEcho( void )
{
	return logEcho;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void LogEcho( bool value )
//|	Date		-	2/03/2003
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Enables/Disables whether console should echo log messages
//o-----------------------------------------------------------------------------------------------o
void CConsole::LogEcho( bool value )
{
	logEcho = value;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void StartOfLineCheck( void )
//|	Date		-	2/3/2003
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Reduces excess work in overloaded functions by checking
//|					to see if the start of line has to be done here
//o-----------------------------------------------------------------------------------------------o
void CConsole::StartOfLineCheck( void )
{
	if( curLeft == 0 )
	{
		PrintStartOfLine();
		curLeft = 1;
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void PrintSpecial( UI08 colour, const std::string& msg )
//|	Date		-	2/7/2003
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	This is much like PrintFailed, PrintDone and so on except
//|					we specify the text and the colour
//o-----------------------------------------------------------------------------------------------o
void CConsole::PrintSpecial( UI08 colour, const std::string& msg )
{
	StartOfLineCheck();
	size_t stringLength = msg.size() + 3;
	MoveTo( static_cast< SI32 >(width - stringLength) );
	TurnNormal();
	(*this) << "[";
	switch( colour )
	{
		default:
		case CNORMAL:						break;
		case CBLUE:		TurnBlue();			break;
		case CRED:		TurnRed();			break;
		case CGREEN:	TurnGreen();		break;
		case CYELLOW:	TurnYellow();		break;
		case CBWHITE:	TurnBrightWhite();	break;
	}
	(*this) << msg;
	TurnNormal();
	(*this) << "]" << myendl;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	SI32 cl_getch( void )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Read a character from stdin, in a cluox compatble way.
//|					This routine is non-blocking!
//|	Returns		-	>0 -> character read
//|					-1 -> no char available.
//o-----------------------------------------------------------------------------------------------o
//|	Notes		-	now cluox is GUI wrapper over uox using stdin and stdout redirection to capture
//|					the console, if it is active uox can't use kbhit() to determine if there is a
//|					character aviable, it can only get one directly by getch().
//|					However the problem arises that uox will get blocked if none is aviable.
//|					The solution to this problem is that cluox also hands over the second pipe-end
//|					of stdin so uox can write itself into this stream. To determine if a character is
//|					now done that way. UOX write's itself a ZERO on the other end of the pipe, and reads
//|					a charecter, if it is again the same ZERO just putted in nothing was entered. However
//|					it is not a zero the user has entered a char.
//o-----------------------------------------------------------------------------------------------o
SI32 CConsole::cl_getch( void )
{
#if UOX_PLATFORM != PLATFORM_WIN32
	// first the linux style, don't change it's behavoir
	UI08 c = 0;
	fd_set KEYBOARD;
	FD_ZERO( &KEYBOARD );
	FD_SET( 0, &KEYBOARD );
	SI32 s = select( 1, &KEYBOARD, NULL, NULL, &cwmWorldState->uoxtimeout );
	if( s < 0 )
	{
		error( format("%c", "Error scanning key press") );
		messageLoop << MSG_SHUTDOWN;
	}
	if( s > 0 )
	{
		read( 0, &c, 1 );
		if( c == 0x0A )
			return -1;
	}
#else
	// now the windows one
	if( !cluox_io )
	{
		// uox is not wrapped simply use the kbhit routine
		if( _kbhit() )
			return _getch();
		else
			return -1;
	}
	// the wiered cluox getter.
	UI08 c = 0;
	unsigned long bytes_written = 0;
	SI32 asw = 0;
	if( !cluox_nopipe_fill )
		asw = WriteFile( cluox_stdin_writeback, &c, 1, &bytes_written, NULL );
	if( bytes_written != 1 || asw == 0 )
	{
		warning( "Using cluox-io" );
		messageLoop << MSG_SHUTDOWN;
	}
	c = (UI08)fgetc( stdin );
	if( c == 0 )
	{
		cluox_nopipe_fill = false;
		return -1;
	}
#endif
	// here an actual charater is read in
	return c;
}

//o-----------------------------------------------------------------------------------------------o
//| Function	-	void Poll( void )
//| Date		-	Unknown
//|	Changes		-	25 January, 2006
//|					Moved into the console class and renamed from checkkey() to Poll()
//o-----------------------------------------------------------------------------------------------o
//| Purpose		-	Facilitate console control. SysOp keys, and localhost
//|					controls.
//o-----------------------------------------------------------------------------------------------o
void CConsole::Poll( void )
{
	SI32 c = cl_getch();
	if( c > 0 )
	{
		if( (cluox_io) && ( c == 250 ) )
		{	// force unsecure mode, need this since cluox can't know
			//      how the toggle status is.
			if( cwmWorldState->GetSecure() )
			{
				(*this) << "Secure mode disabled. Press ? for a commands list." << myendl;
				cwmWorldState->SetSecure( false );
				return;
			}
		}
		c = toupper(c);
		Process( c );
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void Process( SI32 c )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Handle keypresses in console
//|	Changes		-	05042004 - added some console debugging stuff.
//|					Ideally this will all be placed onto a DEBUG menu which
//|					a user will be able to select using the <, and > keys
//|					respectivly. This functionality hasn't been implemented
//|					at the current time of writing, but will be when possible.
//o-----------------------------------------------------------------------------------------------o
void CConsole::Process( SI32 c )
{
	if( c == '*' )
	{
		if( cwmWorldState->GetSecure() )
			messageLoop << "Secure mode disabled. Press ? for a commands list";
		else
			messageLoop << "Secure mode re-enabled";
		cwmWorldState->SetSecure( !cwmWorldState->GetSecure() );
		return;
	}
	else
	{
		if( cwmWorldState->GetSecure() )
		{
			messageLoop << "Secure mode prevents keyboard commands! Press '*' to disable";
			return;
		}

		JSCONSOLEKEYMAP_ITERATOR toFind = JSKeyHandler.find( c );
		if( toFind != JSKeyHandler.end() )
		{
			if( toFind->second.isEnabled )
			{
				cScript *toExecute = JSMapping->GetScript( toFind->second.scriptID );
				if( toExecute != NULL )
				{	// All commands that execute are of the form: command_commandname (to avoid possible clashes)
#if defined( UOX_DEBUG_MODE )
					print(format( "Executing JS keystroke %c %s\n", c, toFind->second.cmdName.c_str()) );
#endif
					toExecute->CallParticularEvent( toFind->second.cmdName.c_str(), NULL, 0 );
				}
				return;
			}
		}
		CSocket *tSock	= NULL;
		//char outputline[128], temp[1024];
		std::string outputline, temp;
		SI32 indexcount	= 0;
		bool kill		= false;
		SI32 j = 0;
		switch( c )
		{
			case '!':
				// Force server to save accounts file
				messageLoop << "CMD: Saving Accounts... ";
				Accounts->Save();
				messageLoop << MSG_PRINTDONE;
				break;
			case '@':
				// Force server to save all files.(Manual save)
				messageLoop << MSG_WORLDSAVE;
				break;
			case 'Y':
				SI32 keyresp;
				std::cout << "System: ";
				while( !kill )
				{
					keyresp = cl_getch();
					switch( keyresp )
					{
						case -1:	// no key pressed
						case 0:
							break;
						case 0x1B:
							outputline = "";
							indexcount = 0;
							kill = true;
							std::cout << std::endl;
							messageLoop << "CMD: System broadcast canceled.";
							break;
						case 0x08:
							--indexcount;
							if( indexcount < 0 )
								indexcount = 0;
							else
								std::cout << "\b \b";
							break;
						case 0x0A:
						case 0x0D:
							outputline[indexcount] = 0;
							messageLoop.NewMessage( MSG_CONSOLEBCAST, outputline.c_str() );
							indexcount = 0;
							kill = true;
							std::cout << std::endl;
							temp=format( "CMD: System broadcast sent message \"%s\"", outputline.c_str() );
							outputline = "";
							messageLoop << temp;
							break;
						default:
							if( static_cast<size_t>(indexcount) < sizeof( outputline ) )
							{
								outputline[indexcount++] = (UI08)(keyresp);
								std::cout << (char)keyresp;
							}
							break;
					}
					keyresp = 0x00;
				}
				break;
			case '[':
			{
				// We want to group all the contents of the multimap container numerically by group. We rely on the self ordering in the multimap implementation to do this.
				messageLoop << "  ";
				messageLoop << "Auto-AddMenu Statistics";
				messageLoop << "  ";
				std::string szBuffer;
				// We need to get an iteration into the map first of all the top level ULONGs then we can get an equal range.
				std::map< UI32, UI08 > localMap;
				localMap.clear();
				for( ADDMENUMAP_CITERATOR CJ = g_mmapAddMenuMap.begin(); CJ != g_mmapAddMenuMap.end(); CJ++ )
				{
					// check to see if the group id has been checked already
					if( localMap.find( CJ->first ) == localMap.end() )
					{
						localMap.insert( std::make_pair( CJ->first, 0 ) );
						szBuffer = "";
						szBuffer=format( "AddMenuGroup %u:", CJ->first );
						messageLoop << szBuffer;
						std::pair< ADDMENUMAP_CITERATOR, ADDMENUMAP_CITERATOR > pairRange = g_mmapAddMenuMap.equal_range( CJ->first );
						SI32 count = 0;
						for( ADDMENUMAP_CITERATOR CI=pairRange.first;CI != pairRange.second; CI++ )
						{
							count++;
						}
						szBuffer = "";
						szBuffer=format( "   Found %i Auto-AddMenu Item(s).", count );
						messageLoop << szBuffer;
					}
				}
				messageLoop << MSG_SECTIONBEGIN;
				break;
			}
			case '<':
				messageLoop << "Function not implemented.";
				break;
			case '>':
				messageLoop << "Function not implemented.";
				break;
			case 0x1B:
			case 'Q':
				messageLoop << MSG_SECTIONBEGIN;
				messageLoop << "CMD: Immediate Shutdown initialized!";
				messageLoop << MSG_SHUTDOWN;
				break;
			case '0':
				if( !cwmWorldState->GetReloadingScripts() )
				{
					cwmWorldState->SetReloadingScripts( true );
					// Reload all the files. If there are issues with these files change the order reloaded from here first.
					cwmWorldState->ServerData()->Load();
					messageLoop << "CMD: Loading All";
					messageLoop << "     Server INI... ";
					// Reload accounts, and update Access.adm if new accounts available.
					messageLoop << "     Loading Accounts... ";
					Accounts->Load();
					messageLoop << MSG_PRINTDONE;
					// Reload Region Files
					messageLoop << "     Loading Regions... ";
					UnloadRegions();
					LoadRegions();
					messageLoop << MSG_PRINTDONE;
					messageLoop << "     Loading Teleport Locations... ";
					LoadTeleportLocations();
					messageLoop << MSG_PRINTDONE;
					// Reload the serve spawn regions
					messageLoop << "     Loading Spawn Regions... ";
					UnloadSpawnRegions();
					LoadSpawnRegions();
					messageLoop << MSG_PRINTDONE;
					// Reload the server command list
					messageLoop << "     Loading commands... ";
					Commands->Load();
					messageLoop << MSG_PRINTDONE;
					// Reload DFN's
					messageLoop << "     Loading Server DFN... ";
					FileLookup->Reload();
					messageLoop << MSG_PRINTDONE;
					// messageLoop access is REQUIRED, as this function is executing in a different thread, so we need thread safety
					messageLoop << "     Loading JSE Scripts... ";

					// Reload the current Spells
					messageLoop << "     Loading spells... ";
					Magic->LoadScript();
					messageLoop << MSG_PRINTDONE;
					// Reload the HTML output templates
					messageLoop << "     Loading HTML Templates... ";
					HTMLTemplates->Unload();
					HTMLTemplates->Load();
					cwmWorldState->SetReloadingScripts( false );
					messageLoop << MSG_PRINTDONE;
				}
				else
					messageLoop << "Server can only load one script at a time";
				break;
			case 'T':
				// Timed shut down(10 minutes)
				messageLoop << "CMD: 10 Minute Server Shutdown Announced(Timed)";
				cwmWorldState->SetEndTime( BuildTimeValue( 600 ) );
				endmessage(0);
				break;
			case  'D':
				// Disconnect account 0 (useful when client crashes)
				for( tSock = Network->LastSocket(); tSock != NULL; tSock = Network->PrevSocket() )
				{
					if( tSock->AcctNo() == 0 )
						Network->Disconnect( tSock );
				}
				messageLoop << "CMD: Socket Disconnected(Account 0).";
				break;
			case 'K':
			{
				for( tSock = Network->FirstSocket(); !Network->FinishedSockets(); tSock = Network->NextSocket() )
				{
					Network->Disconnect( tSock );
				}
				messageLoop << "CMD: All Connections Closed.";
			}
				break;
			case 'P':
			{
				UI32 networkTimeCount	= cwmWorldState->ServerProfile()->NetworkTimeCount();
				UI32 timerTimeCount		= cwmWorldState->ServerProfile()->TimerTimeCount();
				UI32 autoTimeCount		= cwmWorldState->ServerProfile()->AutoTimeCount();
				UI32 loopTimeCount		= cwmWorldState->ServerProfile()->LoopTimeCount();
				// 1/13/2003 - Dreoth - Log Performance Information enhancements
				LogEcho( true );
				log( "--- Starting Performance Dump ---", "performance.log");
				log( "\tPerformance Dump:", "performance.log");
				log( format("\tNetwork code: %.2fmsec [%i samples]",(R32)((R32)cwmWorldState->ServerProfile()->NetworkTime()/(R32)networkTimeCount), networkTimeCount), "performance.log" );
				log( format("\tTimer code: %.2fmsec [%i samples]", (R32)((R32)cwmWorldState->ServerProfile()->TimerTime()/(R32)timerTimeCount), timerTimeCount), "performance.log");
				log( format("\tAuto code: %.2fmsec [%i samples]", (R32)((R32)cwmWorldState->ServerProfile()->AutoTime()/(R32)autoTimeCount), autoTimeCount), "performance.log");
				log( format("\tLoop Time: %.2fmsec [%i samples]", (R32)((R32)cwmWorldState->ServerProfile()->LoopTime()/(R32)loopTimeCount), loopTimeCount), "performance.log");

				log( format("\tCharacters: %i/%i - Items: %i/%i (Dynamic)", ObjectFactory::getSingleton().CountOfObjects( OT_CHAR ), ObjectFactory::getSingleton().SizeOfObjects( OT_CHAR ), ObjectFactory::getSingleton().CountOfObjects( OT_ITEM ), ObjectFactory::getSingleton().SizeOfObjects( OT_ITEM )), "performance.log" );
				log( format("\tSimulation Cycles: %f per sec", (1000.0*(1.0/(R32)((R32)cwmWorldState->ServerProfile()->LoopTime()/(R32)loopTimeCount)))), "performance.log");
				log( format("\tBytes sent: %i", cwmWorldState->ServerProfile()->GlobalSent()), "performance.log");
				log( format("\tBytes Received: %i", cwmWorldState->ServerProfile()->GlobalReceived()), "performance.log");
				log( "--- Performance Dump Complete ---", "performance.log");
				LogEcho( false );
				break;
			}
			case 'W':
			{
				// Display logged in chars
				messageLoop << "CMD: Current Users in the World:";
				CSocket *iSock;
				{	//std::scoped_lock lock(Network->internallock);
					Network->pushConn();

					for( iSock = Network->FirstSocket(); !Network->FinishedSockets(); iSock = Network->NextSocket() )
					{
						++j;
						CChar *mChar = iSock->CurrcharObj();

						temp = format( "     %i) %s [%x %x %x %x]", j - 1, mChar->GetName().c_str(), mChar->GetSerial( 1 ), mChar->GetSerial( 2 ), mChar->GetSerial( 3 ), mChar->GetSerial( 4 ) );
						messageLoop << temp;
					}
					Network->popConn();
				}

				temp = format( "     Total users online: %i", j );
				messageLoop << temp;
				break;
			}
			case 'M':
				UI32 tmp, total;
				total = 0;
				tmp = 0;
				messageLoop << "CMD: UOX Memory Information:";
				messageLoop << "     Cache:";
				temp = format( "        Tiles: %zu bytes", Map->GetTileMem() );
				messageLoop << temp;
				temp = format( "        Multis: %zu bytes", Map->GetMultisMem() );
				messageLoop << temp;
				UI32 m, n;
				m = static_cast<std::uint32_t>(ObjectFactory::getSingleton().SizeOfObjects( OT_CHAR ));
				total += tmp = m + m*sizeof( CTEffect ) + m*sizeof(char) + m*sizeof( intptr_t )*5;
				temp = format( "     Characters: %u bytes [%u chars ( %u allocated )]", tmp, ObjectFactory::getSingleton().CountOfObjects( OT_CHAR ), m );
				messageLoop << temp;
				n = static_cast<std::uint32_t>(ObjectFactory::getSingleton().SizeOfObjects( OT_ITEM ));
				total += tmp = n + n * sizeof( intptr_t ) * 4;
				temp = format( "     Items: %u bytes [%u items ( %u allocated )]", tmp, ObjectFactory::getSingleton().CountOfObjects( OT_ITEM ), n );
				messageLoop << temp;
				temp = format( "        You save I: %lu & C: %lu bytes!", m * sizeof(CItem) - ObjectFactory::getSingleton().CountOfObjects( OT_ITEM ), m * sizeof( CChar ) - ObjectFactory::getSingleton().CountOfObjects( OT_CHAR ) );
				total += tmp = 69 * sizeof( SpellInfo );
				temp = format( temp, "     Spells: %i bytes", tmp );
				messageLoop << "     Sizes:";
				temp = format("        CItem  : %lu bytes", sizeof( CItem ) );
				messageLoop << temp;
				temp = format( "        CChar  : %lu bytes", sizeof( CChar ) );
				messageLoop << temp;
				temp = format( "        TEffect: %lu bytes %lui total)", sizeof( CTEffect ), sizeof( CTEffect ) * cwmWorldState->tempEffects.Num() );
				messageLoop << temp;
				tmp = static_cast<std::uint32_t>(Map->GetTileMem() + Map->GetMultisMem());
				total += tmp;
				temp = format( "        Approximate Total: %i bytes", total );
				messageLoop << temp;
				break;
			case '?':
				messageLoop << MSG_SECTIONBEGIN;
				messageLoop << "Console commands:";
				messageLoop << MSG_SECTIONBEGIN;
				messageLoop << " ShardOP:";
				messageLoop << "    * - Lock/Unlock Console ? - Commands list(this)";
				messageLoop << "    C - Configuration       H - Unused";
				messageLoop << "    Y - Console Broadcast   Q - Quit/Exit           ";
				messageLoop << " Load Commands:";
				messageLoop << "    1 - Ini                 2 - Accounts";
				messageLoop << "    3 - Regions             4 - Spawn Regions";
				messageLoop << "    5 - Spells              6 - Commands";
				messageLoop << "    7 - Dfn's               8 - JavaScript";
				messageLoop << "    9 - HTML Templates      0 - ALL(1-9)";
				messageLoop << " Save Commands:";
				messageLoop << "    ! - Accounts            @ - World(w/AccountImport)";
				messageLoop << "    # - Unused              $ - Unused";
				messageLoop << "    % - Unused              ^ - Unused";
				messageLoop << "    & - Unused              ( - Unused";
				messageLoop << "    ) - Unused";
				messageLoop << " Server Maintenence:";
				messageLoop << "    P - Performance         W - Characters Online";
				messageLoop << "    M - Memory Information  T - 10 Minute Shutdown";
				messageLoop << "    F - Display Priority Maps";
				messageLoop << " Network Maintenence:";
				messageLoop << "    D - Disconnect Acct0    K - Disconnect All";
				messageLoop << "    Z - Socket Logging      ";
				messageLoop << MSG_SECTIONBEGIN;
				break;
			case 'z':
			case 'Z':
			{
				bool loggingEnabled = false;
				{
					//std::scoped_lock lock(Network->internallock);
					// Log socket activity
					Network->pushConn();

					CSocket *snSock		= Network->FirstSocket();
					if( snSock != NULL )
						loggingEnabled = !snSock->Logging();
					for( ; !Network->FinishedSockets(); snSock = Network->NextSocket() )
					{
						if( snSock != NULL )
							snSock->Logging( !snSock->Logging() );
					}
					Network->popConn();
				}
				if( loggingEnabled )
					messageLoop << "CMD: Network Logging Enabled.";
				else
					messageLoop << "CMD: Network Logging Disabled.";
				break;
			}
			case 'c':
			case 'C':
				// Shows a configuration header
				DisplaySettings();
				break;
			case 'f':
			case 'F':
				FileLookup->DisplayPriorityMap();
				break;
			default:
				temp = format( "Key \'%c\' [%i] does not perform a function", (char)c, c );
				messageLoop << temp;
				break;
		}
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void Cloak( char *callback )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Cloak UOX3 console within CLUOX
//o-----------------------------------------------------------------------------------------------o
void CConsole::Cloak( char *callback )
{
	(*this) << "Using CLUOX Streaming-IO" << myendl;
	setvbuf( stdout, NULL, _IONBF, 0 );
	setvbuf( stderr, NULL, _IONBF, 0 );
	cluox_io = true;
	char *dummy;
	cluox_stdin_writeback = (void *)strtol( callback, &dummy, 16 );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void DisplaySettings( void )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	UOX startup stuff
//|					Moved that here because we need it in processkey now
//|
//|	Changes		-	10/21/2002 - found the bug in one spot, just
//|									happened upon this quick fix. for BackUp operation.
//o-----------------------------------------------------------------------------------------------o
void CConsole::DisplaySettings( void )
{
	std::map< bool, std::string > activeMap;
	activeMap[true] = "Activated!";
	activeMap[false] = "Disabled!";

	// UOX.ini status
	(*this) << "Server Settings:" << myendl;

	(*this) << "   -Archiving[";
	if( cwmWorldState->ServerData()->ServerBackupStatus() )
		(*this) << "Enabled]. (" << cwmWorldState->ServerData()->Directory( CSDDP_BACKUP ) << ")" << myendl;
	else
		(*this) << "Disabled]" << myendl;

	(*this) << "   -Weapons & Armour Rank System: ";
	(*this) << activeMap[cwmWorldState->ServerData()->RankSystemStatus()] << myendl;

	(*this) << "   -Vendors buy by item name: ";
	(*this) << activeMap[cwmWorldState->ServerData()->SellByNameStatus()] << myendl;

	(*this) << "   -Adv. Trade System: ";
	(*this) << activeMap[cwmWorldState->ServerData()->TradeSystemStatus()] << myendl;


	(*this) << "   -Races: " << static_cast< UI32 >(Races->Count()) << myendl;
	(*this) << "   -Guilds: " << static_cast< UI32 >(GuildSys->NumGuilds()) << myendl;
	(*this) << "   -Char count: " << ObjectFactory::getSingleton().CountOfObjects( OT_CHAR ) << myendl;
	(*this) << "   -Item count: " << ObjectFactory::getSingleton().CountOfObjects( OT_ITEM ) << myendl;
	(*this) << "   -Num Accounts: " << Accounts->size() << myendl;
	(*this) << "   Directories: " << myendl;
	(*this) << "   -Shared:          " << cwmWorldState->ServerData()->Directory( CSDDP_SHARED ) << myendl;
	(*this) << "   -Archive:         " << cwmWorldState->ServerData()->Directory( CSDDP_BACKUP ) << myendl;
	(*this) << "   -Data:            " << cwmWorldState->ServerData()->Directory( CSDDP_DATA ) << myendl;
	(*this) << "   -Defs:            " << cwmWorldState->ServerData()->Directory( CSDDP_DEFS ) << myendl;
	(*this) << "   -Scripts:         " << cwmWorldState->ServerData()->Directory( CSDDP_SCRIPTS ) << myendl;
	(*this) << "   -HTML:            " << cwmWorldState->ServerData()->Directory( CSDDP_HTML ) << myendl;
	(*this) << "   -Books:           " << cwmWorldState->ServerData()->Directory( CSDDP_BOOKS ) << myendl;
	(*this) << "   -MessageBoards:   " << cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ) << myendl;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void RegisterKey( SI32 key, std::string cmdName, UI16 scriptID )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Registers key input for detection in console
//o-----------------------------------------------------------------------------------------------o
void CConsole::RegisterKey( SI32 key, std::string cmdName, UI16 scriptID )
{
#if defined( UOX_DEBUG_MODE )

	messageLoop << format("         Registering key \"%c\"", key );
#endif
	JSKeyHandler[key] = JSConsoleEntry( scriptID, cmdName );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void SetKeyStatus( SI32 key, bool isEnabled )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Enabling/Disabling specific key input in console
//o-----------------------------------------------------------------------------------------------o
void CConsole::SetKeyStatus( SI32 key, bool isEnabled )
{
	JSCONSOLEKEYMAP_ITERATOR	toFind	= JSKeyHandler.find( key );
	if( toFind != JSKeyHandler.end() )
	{
		toFind->second.isEnabled = isEnabled;
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void RegisterFunc( const std::string &cmdFunc, const std::string &cmdName, UI16 scriptID )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Registers console function
//o-----------------------------------------------------------------------------------------------o
void CConsole::RegisterFunc( const std::string &cmdFunc, const std::string &cmdName, UI16 scriptID )
{
#if defined( UOX_DEBUG_MODE )
	print(format( "         Registering console func \"%s\"\n", cmdFunc.c_str() ));
#endif
	JSConsoleFunctions[str_toupper(cmdFunc)]	= JSConsoleEntry( scriptID, cmdName );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void SetFuncStatus( const std::string &cmdFunc, bool isEnabled )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Enables/disables console function
//o-----------------------------------------------------------------------------------------------o
void CConsole::SetFuncStatus( const std::string &cmdFunc, bool isEnabled )
{
	UString upper						= cmdFunc;
	upper								= upper.upper();
	JSCONSOLEFUNCMAP_ITERATOR	toFind	= JSConsoleFunctions.find( upper );
	if( toFind != JSConsoleFunctions.end() )
	{
		toFind->second.isEnabled = isEnabled;
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	void Registration( void )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Registers console script
//o-----------------------------------------------------------------------------------------------o
void CConsole::Registration( void )
{
	CJSMappingSection *spellSection = JSMapping->GetSection( SCPT_CONSOLE );
	for( cScript *ourScript = spellSection->First(); !spellSection->Finished(); ourScript = spellSection->Next() )
	{
		if( ourScript != NULL )
			ourScript->ScriptRegistration( "Console" );
	}
}
