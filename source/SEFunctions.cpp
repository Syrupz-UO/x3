// Here are the functions that are exposed to the Script Engine
#include "uox3.h"
#include "cdice.h"
#include "SEFunctions.h"
#include "cGuild.h"
#include "combat.h"
#include "movement.h"
#include "townregion.h"
#include "cWeather.hpp"
#include "cRaces.h"
#include "skills.h"
#include "commands.h"
#include "cMagic.h"
#include "CJSMapping.h"
#include "cScript.h"
#include "cEffects.h"
#include "classes.h"
#include "regions.h"
#include "magic.h"
#include "ssection.h"
#include "cThreadQueue.h"
#include "cHTMLSystem.h"
#include "cServerDefinitions.h"
#include "Dictionary.h"
#include "speech.h"
#include "gump.h"
#include "ObjectFactory.h"
#include "network.h"
#include "UOXJSClasses.h"
#include "UOXJSPropertySpecs.h"
#include "JSEncapsulate.h"
#include "CJSEngine.h"
#include "PartySystem.h"
#include "cSpawnRegion.h"


void		LoadTeleportLocations( void );
void		LoadSpawnRegions( void );
void		LoadRegions( void );
void		UnloadRegions( void );
void		UnloadSpawnRegions( void );
void		LoadSkills( void );

#define __EXTREMELY_VERBOSE__

#ifdef __EXTREMELY_VERBOSE__
void DoSEErrorMessage( const std::string& txt )
{
	if (!txt.empty()){
		auto msg = txt ;
		if (msg.size()>512){
			msg = msg.substr(0,512);
			Console.error( msg );
		}

	}

}
#else
void DoSEErrorMessage( const std::string& txt )
{
	return;
}
#endif

std::map< std::string, ObjectType > stringToObjType;

void InitStringToObjType( void )
{
	stringToObjType["BASEOBJ"]		= OT_CBO;
	stringToObjType["CHARACTER"]	= OT_CHAR;
	stringToObjType["ITEM"]			= OT_ITEM;
	stringToObjType["SPAWNER"]		= OT_SPAWNER;
	stringToObjType["MULTI"]		= OT_MULTI;
	stringToObjType["BOAT"]			= OT_BOAT;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	ObjectType FindObjTypeFromString( UString strToFind )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Determine object type (ITEM, CHARACTER, MULTI, etc) based on provided string
//o-----------------------------------------------------------------------------------------------o
ObjectType FindObjTypeFromString( UString strToFind )
{
	if( stringToObjType.empty() )	// if we haven't built our array yet
		InitStringToObjType();
	std::map< std::string, ObjectType >::const_iterator toFind = stringToObjType.find( strToFind.upper() );
	if( toFind != stringToObjType.end() )
		return toFind->second;
	return OT_CBO;
}

// Effect related functions
//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_DoTempEffect( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Does a temporary effect (things like protection, night sight, and what not) frm
//|					src to trg. If iType = 0, then it's a character, otherwise it's an item.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_DoTempEffect( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 7 )
	{
		DoSEErrorMessage( "DoTempEffect: Invalid number of arguments (takes 7 or 8)" );
		return JS_FALSE;
	}
	UI08 iType			= static_cast<UI08>(JSVAL_TO_INT( argv[0] ));
	UI32 targNum		= JSVAL_TO_INT( argv[3] );
	UI08 more1			= (UI08)JSVAL_TO_INT( argv[4] );
	UI08 more2			= (UI08)JSVAL_TO_INT( argv[5] );
	UI08 more3			= (UI08)JSVAL_TO_INT( argv[6] );

	CItem *myItemPtr	= NULL;

	if( argc == 8 )
	{
		JSObject *myitemptr = NULL;
		myitemptr = JSVAL_TO_OBJECT( argv[7] );
		myItemPtr = static_cast<CItem *>(JS_GetPrivate( cx, myitemptr ));
	}

	JSObject *mysrc		= JSVAL_TO_OBJECT( argv[1] );
	CChar *mysrcChar	= static_cast<CChar*>(JS_GetPrivate( cx, mysrc ));

	if( !ValidateObject( mysrcChar ) )
	{
		DoSEErrorMessage( "DoTempEffect: Invalid src" );
		return JS_FALSE;
	}

	if( iType == 0 )	// character
	{
		JSObject *mydestc = JSVAL_TO_OBJECT( argv[2] );
		CChar *mydestChar = static_cast<CChar*>(JS_GetPrivate( cx, mydestc ));

		if( !ValidateObject( mydestChar ) )
		{
			DoSEErrorMessage( "DoTempEffect: Invalid target " );
			return JS_FALSE;
		}
		if( argc == 8 )
			Effects->tempeffect( mysrcChar, mydestChar, static_cast<SI08>(targNum), more1, more2, more3, myItemPtr );
		else
			Effects->tempeffect( mysrcChar, mydestChar, static_cast<SI08>(targNum), more1, more2, more3 );
	}
	else
	{
		JSObject *mydesti = JSVAL_TO_OBJECT( argv[2] );
		CItem *mydestItem = static_cast<CItem *>(JS_GetPrivate( cx, mydesti ));

		if( !ValidateObject( mydestItem ) )
		{
			DoSEErrorMessage( "DoTempEffect: Invalid target " );
			return JS_FALSE;
		}
		Effects->tempeffect( mysrcChar, mydestItem, static_cast<SI08>(targNum), more1, more2, more3 );
	}
	return JS_TRUE;
}

// Speech related functions
void sysBroadcast( const std::string& txt );
//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_BroadcastMessage( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Broadcasts specified string to all online players
//o-----------------------------------------------------------------------------------------------o
JSBool SE_BroadcastMessage( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "BroadcastMessage: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string trgMessage = JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	if( trgMessage.empty() || trgMessage.length() == 0 )
	{
		DoSEErrorMessage( format("BroadcastMessage: Invalid string (%s)", trgMessage.c_str()) );
		return JS_FALSE;
	}
	sysBroadcast( trgMessage );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CalcItemFromSer( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Calculates and returns item object based on provided serial
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CalcItemFromSer( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 && argc != 4 )
	{
		DoSEErrorMessage( "CalcItemFromSer: Invalid number of arguments (takes 1 or 4)" );
		return JS_FALSE;
	}
	SERIAL targSerial;
	if( argc == 1 )
		targSerial = (SERIAL)JSVAL_TO_INT( argv[0] );
	else
		targSerial = calcserial( (UI08)JSVAL_TO_INT( argv[0] ), (UI08)JSVAL_TO_INT( argv[1] ), (UI08)JSVAL_TO_INT( argv[2] ), (UI08)JSVAL_TO_INT( argv[3] ) );

	CItem *newItem		= calcItemObjFromSer( targSerial );
	if( newItem != NULL )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, newItem, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CalcCharFromSer( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Calculates and returns character object based on provided serial
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CalcCharFromSer( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 && argc != 4 )
	{
		DoSEErrorMessage( "CalcCharFromSer: Invalid number of arguments (takes 1 or 4)" );
		return JS_FALSE;
	}
	SERIAL targSerial = INVALIDSERIAL;
	if( argc == 1 )
		targSerial = (SERIAL)JSVAL_TO_INT( argv[0] );
	else
		targSerial = calcserial( (UI08)JSVAL_TO_INT( argv[0] ), (UI08)JSVAL_TO_INT( argv[1] ), (UI08)JSVAL_TO_INT( argv[2] ), (UI08)JSVAL_TO_INT( argv[3] ) );

	CChar *newChar		= calcCharObjFromSer( targSerial );
	if( newChar != NULL )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_CHAR, newChar, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_DoMovingEffect( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Plays a moving effect from source object to target object or location
//o-----------------------------------------------------------------------------------------------o
JSBool SE_DoMovingEffect( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 6 )
	{
		DoSEErrorMessage( "DoMovingEffect: Invalid number of arguments (takes 6->8 or 8->10)" );
		return JS_FALSE;
	}
	JSObject *srcObj	= JSVAL_TO_OBJECT( argv[0] );
	CBaseObject *src	= static_cast<CBaseObject *>(JS_GetPrivate( cx, srcObj ));
	if( !ValidateObject( src ) )
	{
		DoSEErrorMessage( "DoMovingEffect: Invalid source object" );
		return JS_FALSE;
	}
	bool targLocation	= false;
	UI08 offset			= 0;
	UI16 targX			= 0;
	UI16 targY			= 0;
	SI08 targZ			= 0;
	CBaseObject *trg	= NULL;
	if( JSVAL_IS_INT( argv[1] ) )
	{	// Location moving effect
		targLocation	= true;
		offset			= true;
		targX			= (UI16)JSVAL_TO_INT( argv[1] );
		targY			= (UI16)JSVAL_TO_INT( argv[2] );
		targZ			= (SI08)JSVAL_TO_INT( argv[3] );
	}
	else
	{
		JSObject *trgObj	= JSVAL_TO_OBJECT( argv[1] );
		trg					= static_cast<CBaseObject *>(JS_GetPrivate( cx, trgObj ));
		if( !ValidateObject( trg ) )
		{
			DoSEErrorMessage( "DoMovingEffect: Invalid target object" );
			return JS_FALSE;
		}
	}
	UI16 effect		= (UI16)JSVAL_TO_INT( argv[2+offset] );
	UI08 speed		= (UI08)JSVAL_TO_INT( argv[3+offset] );
	UI08 loop		= (UI08)JSVAL_TO_INT( argv[4+offset] );
	bool explode	= ( JSVAL_TO_BOOLEAN( argv[5+offset] ) == JS_TRUE );
	UI32 hue = 0, renderMode = 0;
	if( argc - offset >= 7 ) // there's at least 7/9 parameters
		hue = (UI32)JSVAL_TO_INT( argv[6+offset] );
	if( argc - offset >= 8 ) // there's at least 8/10 parameters
		renderMode = (UI32)JSVAL_TO_INT( argv[7+offset] );

	if( targLocation )
		Effects->PlayMovingAnimation( src, targX, targY, targZ, effect, speed, loop, explode, hue, renderMode );
	else
		Effects->PlayMovingAnimation( src, trg, effect, speed, loop, explode, hue, renderMode );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RandomNumber( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns a random number between loNum and hiNum
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RandomNumber( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "RandomNumber: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	JSEncapsulate loVal( cx, &(argv[0]) );
	JSEncapsulate hiVal( cx, &(argv[1]) );
	*rval = INT_TO_JSVAL( RandomNum( loVal.toInt(), hiVal.toInt() ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_MakeItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Character creates specified item based on entry in CREATE DFNs
//o-----------------------------------------------------------------------------------------------o
JSBool SE_MakeItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 3 )
	{
		DoSEErrorMessage( "MakeItem: Invalid number of arguments (takes 3)" );
		return JS_FALSE;
	}
	JSObject *mSock = JSVAL_TO_OBJECT( argv[0] );
	CSocket *sock	= static_cast<CSocket *>(JS_GetPrivate( cx, mSock ));
	JSObject *mChar = JSVAL_TO_OBJECT( argv[1] );
	CChar *player	= static_cast<CChar *>(JS_GetPrivate( cx, mChar ));
	if( !ValidateObject( player ) )
	{
		DoSEErrorMessage( "MakeItem: Invalid character" );
		return JS_FALSE;
	}
	UI16 itemMenu		= (UI16)JSVAL_TO_INT( argv[2] );
	createEntry *toFind = Skills->FindItem( itemMenu );
	if( toFind == NULL )
	{
		DoSEErrorMessage( format("MakeItem: Invalid make item (%i)", itemMenu) );
		return JS_FALSE;
	}
	Skills->MakeItem( *toFind, player, sock, itemMenu );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CommandLevelReq( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the command level required to execute the specified command
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CommandLevelReq( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "CommandLevelReq: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string test = JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	if( test.empty() || test.length() == 0 )
	{
		DoSEErrorMessage( "CommandLevelReq: Invalid command name" );
		return JS_FALSE;
	}
	CommandMapEntry *details = Commands->CommandDetails( test );
	if( details == NULL )
		*rval = INT_TO_JSVAL( 255 );
	else
		*rval = INT_TO_JSVAL( details->cmdLevelReq );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CommandExists( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns whether specified command exists in command table or not
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CommandExists( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "CommandExists: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string test = JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	if( test.empty() || test.length() == 0 )
	{
		DoSEErrorMessage( "CommandExists: Invalid command name" );
		return JS_FALSE;
	}
	*rval = BOOLEAN_TO_JSVAL( Commands->CommandExists( test ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_FirstCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the name of the first command in the table. If nothing, it's a 0 length string.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_FirstCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	const std::string tVal = Commands->FirstCommand();
	JSString *strSpeech = NULL;
	if( tVal.empty() )
		strSpeech = JS_NewStringCopyZ( cx, "" );
	else
		strSpeech = JS_NewStringCopyZ( cx, tVal.c_str() );

	*rval = STRING_TO_JSVAL( strSpeech );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_NextCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the name of the next command in the table. If nothing, it's a 0 length string.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_NextCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	const std::string tVal = Commands->NextCommand();
	JSString *strSpeech = NULL;
	if( tVal.empty() )
		strSpeech = JS_NewStringCopyZ( cx, "" );
	else
		strSpeech = JS_NewStringCopyZ( cx, tVal.c_str() );

	*rval = STRING_TO_JSVAL( strSpeech );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_FinishedCommandList( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns true if there are no more commands left in the table.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_FinishedCommandList( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	*rval = BOOLEAN_TO_JSVAL( Commands->FinishedCommandList() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RegisterCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	If called from within CommandRegistration() function in a script registered
//|					under the COMMAND_SCRIPTS section of JSE_FILEASSOCIATIONS.SCP, will register
//|					the specified command in the command table and call the function in the same
//|					script whose name corresponds with the command name, in the shape of
//|						function command_CMDNAME( socket, cmdString )
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RegisterCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 3 )
	{
		DoSEErrorMessage( "  RegisterCommand: Invalid number of arguments (takes 4)" );
		return JS_FALSE;
	}
	std::string toRegister	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	UI08 execLevel			= static_cast<UI08>(JSVAL_TO_INT( argv[1] ));
	bool isEnabled			= ( JSVAL_TO_BOOLEAN( argv[2] ) == JS_TRUE );
	UI16 scriptID			= JSMapping->GetScriptID( JS_GetGlobalObject( cx ) );

	if( scriptID == 0xFFFF )
	{
		DoSEErrorMessage( " RegisterCommand: JS Script has an Invalid ScriptID" );
		return JS_FALSE;
	}

	Commands->Register( toRegister, scriptID, execLevel, isEnabled );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RegisterSpell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	If called from within SpellRegistration() function in a script registered under
//|					the MAGIC_SCRIPTS section of JSE_FILEASSOCIATIONS.SCP, will register the
//|					onSpellCast() event in the same script as a global listener for use of the
//|					specified magic spell.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RegisterSpell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "RegisterSpell: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	SI32 spellNumber			= JSVAL_TO_INT( argv[0] );
	bool isEnabled			= ( JSVAL_TO_BOOLEAN( argv[1] ) == JS_TRUE );
	cScript *myScript		= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
	Magic->Register( myScript, spellNumber, isEnabled );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RegisterSkill( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Register JS script as a global listener for use of specified skill, and
//|					triggers onSkill() event in same script when specified skill is used, if
//|					script is added under the SKILLUSE_SCRIPTS section of JSE_FILEASSOCIATIONS.SCP
//|					and this function is called from a SkillRegistration() function
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RegisterSkill( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "RegisterSkill: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	SI32 skillNumber			= JSVAL_TO_INT( argv[0] );
	bool isEnabled			= ( JSVAL_TO_BOOLEAN( argv[1] ) == JS_TRUE );
	UI16 scriptID			= JSMapping->GetScriptID( JS_GetGlobalObject( cx ) );
	if( scriptID != 0xFFFF )
	{
#if defined( UOX_DEBUG_MODE )
		Console.print( " " );
		Console.MoveTo( 15 );
		Console.print( "Registering skill number " );
		Console.TurnYellow();
		Console.print( format("%i", skillNumber ));
		if( !isEnabled )
		{
			Console.TurnRed();
			Console.print( " [DISABLED]" );
		}
		Console.print( "\n" );
		Console.TurnNormal();
#endif
		// If skill is not enabled, unset scriptID from skill data
		if( !isEnabled )
		{
			cwmWorldState->skill[skillNumber].jsScript = 0;
			return JS_FALSE;
		}

		// Skillnumber above ALLSKILLS refers to STR, INT, DEX, Fame and Karma,
		if( skillNumber < 0 || skillNumber >= ALLSKILLS )
			return JS_TRUE;

		// Both scriptID and skillNumber seem valid; assign scriptID to this skill
		cwmWorldState->skill[skillNumber].jsScript = scriptID;
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RegisterPacket( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Register JS script as a global listener for a specific network packet, and
//|					triggers onPacketReceive() event in same script when this network packet is sent,
//|					if script is added under the PACKET_SCRIPTS section of JSE_FILEASSOCIATIONS.SCP
//|					and this function is called from a PacketRegistration() function
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RegisterPacket( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "RegisterPacket: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	UI08 packet			= static_cast<UI08>(JSVAL_TO_INT( argv[0] ));
	UI08 subCmd			= static_cast<UI08>(JSVAL_TO_INT( argv[1] ));
	UI16 scriptID		= JSMapping->GetScriptID( JS_GetGlobalObject( cx ) );
	if( scriptID != 0xFFFF )
	{
#if defined( UOX_DEBUG_MODE )
		Console.print( format("Registering packet number 0x%X, subcommand 0x%x\n", packet, subCmd) );
#endif
		Network->RegisterPacket( packet, subCmd, scriptID );
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RegisterKey( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Register JS script as a global listener for a specific keypress in UOX3 console,
//|					and triggers specified function in same script when key is pressed, if script
//|					is added under the CONSOLE_SCRIPTS section of JSE_FILEASSOCIATIONS.SCP
//|					and this function is called from a ConsoleRegistration() function
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RegisterKey( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "RegisterKey: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	JSEncapsulate encaps( cx, &(argv[0]) );
	std::string toRegister	= JS_GetStringBytes( JS_ValueToString( cx, argv[1] ) );
	UI16 scriptID			= JSMapping->GetScriptID( JS_GetGlobalObject( cx ) );

	if( scriptID == 0xFFFF )
	{
		DoSEErrorMessage( "RegisterKey: JS Script has an Invalid ScriptID" );
		return JS_FALSE;
	}
	SI32 toPass = 0;
	if( encaps.isType( JSOT_STRING ) )
	{
		std::string enStr = encaps.toString();
		if( enStr.length() != 0 )
			toPass  = enStr[0];
		else
		{
			DoSEErrorMessage( "RegisterKey: JS Script passed an invalid key to register" );
			return JS_FALSE;
		}
	}
	else
		toPass = encaps.toInt();
	Console.RegisterKey( toPass, toRegister, scriptID );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RegisterConsoleFunc( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	???
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RegisterConsoleFunc( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "RegisterConsoleFunc: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	std::string funcToRegister	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	std::string toRegister		= JS_GetStringBytes( JS_ValueToString( cx, argv[1] ) );
	UI16 scriptID				= JSMapping->GetScriptID( JS_GetGlobalObject( cx ) );

	if( scriptID == 0xFFFF )
	{
		DoSEErrorMessage( "RegisterConsoleFunc: JS Script has an Invalid ScriptID" );
		return JS_FALSE;
	}

	Console.RegisterFunc( funcToRegister, toRegister, scriptID );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_DisableCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Disables a specified command on the server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_DisableCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "DisableCommand: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string toDisable	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	Commands->SetCommandStatus( toDisable, false );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_DisableKey( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Disables specified key in console
//o-----------------------------------------------------------------------------------------------o
JSBool SE_DisableKey( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "DisableKey: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	SI32 toDisable = JSVAL_TO_INT( argv[0] );
	Console.SetKeyStatus( toDisable, false );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_DisableConsoleFunc( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	???
//o-----------------------------------------------------------------------------------------------o
JSBool SE_DisableConsoleFunc( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "DisableConsoleFunc: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string toDisable	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	Console.SetFuncStatus( toDisable, false );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_DisableSpell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Disables specified spell on server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_DisableSpell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "DisableSpell: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	SI32 spellNumber = JSVAL_TO_INT( argv[0] );
	Magic->SetSpellStatus( spellNumber, false );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_EnableCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Enables specified command on server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_EnableCommand( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "EnableCommand: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string toEnable	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	Commands->SetCommandStatus( toEnable, true );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_EnableSpell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Enables specified spell on server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_EnableSpell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "EnableSpell: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	SI32 spellNumber = JSVAL_TO_INT( argv[0] );
	Magic->SetSpellStatus( spellNumber, true );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_EnableKey( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Enables specified key in console
//o-----------------------------------------------------------------------------------------------o
JSBool SE_EnableKey( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "EnableKey: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	SI32 toEnable = JSVAL_TO_INT( argv[0] );
	Console.SetKeyStatus( toEnable, true );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_EnableConsoleFunc( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	???
//o-----------------------------------------------------------------------------------------------o
JSBool SE_EnableConsoleFunc( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "EnableConsoleFunc: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	std::string toEnable	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	Console.SetFuncStatus( toEnable, false );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetHour( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the hour of the current UO day
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetHour( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	bool ampm = cwmWorldState->ServerData()->ServerTimeAMPM();
	UI08 hour = cwmWorldState->ServerData()->ServerTimeHours();
	if( ampm )
		*rval = INT_TO_JSVAL( hour + 12 );
	else
		*rval = INT_TO_JSVAL( hour );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetMinute( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the minute of the current UO day
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetMinute( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	UI08 minute = cwmWorldState->ServerData()->ServerTimeMinutes();
	*rval = INT_TO_JSVAL( minute );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetDay( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the day number of the server (UO days since server start)
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetDay( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	SI16 day = cwmWorldState->ServerData()->ServerTimeDay();
	*rval = INT_TO_JSVAL( day );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_SecondsPerUOMinute( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets and sets the amonut of real life seconds associated with minute in the game
//o-----------------------------------------------------------------------------------------------o
JSBool SE_SecondsPerUOMinute( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 1 )
	{
		DoSEErrorMessage( "SecondsPerUOMinute: Invalid number of arguments (takes 0 or 1)" );
		return JS_FALSE;
	}
	else if( argc == 1 )
	{
		UI16 secondsPerUOMinute = (UI16)JSVAL_TO_INT( argv[0] );
		cwmWorldState->ServerData()->ServerSecondsPerUOMinute( secondsPerUOMinute );
	}
	*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ServerSecondsPerUOMinute() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetCurrentClock( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets current server clock, aka number of clock ticks since server startup
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetCurrentClock( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	*rval = INT_TO_JSVAL( cwmWorldState->GetUICurrentTime() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_SpawnNPC( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Spawns NPC based on definition in NPC DFNs
//o-----------------------------------------------------------------------------------------------o
JSBool SE_SpawnNPC( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 5 || argc > 6 )
	{
		DoSEErrorMessage( "SpawnNPC: Invalid number of arguments (takes 5 or 6)" );
		return JS_FALSE;
	}

	CChar *cMade		= NULL;
	std::string nnpcNum	= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	UI16 x				= (UI16)JSVAL_TO_INT( argv[1] );
	UI16 y				= (UI16)JSVAL_TO_INT( argv[2] );
	SI08 z				= (SI08)JSVAL_TO_INT( argv[3] );
	UI08 world			= (UI08)JSVAL_TO_INT( argv[4] );
	UI16 instanceID = ( argc == 6 ? (SI16)JSVAL_TO_INT( argv[5] ) : 0 );

	cMade				= Npcs->CreateNPCxyz( nnpcNum, x, y, z, world, instanceID );
	if( cMade != NULL )
	{
		JSObject *myobj		= JSEngine->AcquireObject( IUE_CHAR, cMade, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval				= OBJECT_TO_JSVAL( myobj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CreateDFNItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Creates item based on definition in item DFNs
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CreateDFNItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 3 )
	{
		DoSEErrorMessage( "CreateDFNItem: Invalid number of arguments (takes at least 3)" );
		return JS_FALSE;
	}

	CSocket *mySock = NULL;
	if( argv[0] != JSVAL_NULL )
	{
		JSObject *mSock			= JSVAL_TO_OBJECT( argv[0] );
		mySock					= static_cast<CSocket *>(JS_GetPrivate( cx, mSock ));
	}

	JSObject *mChar				= JSVAL_TO_OBJECT( argv[1] );
	CChar *myChar				= static_cast<CChar *>(JS_GetPrivate( cx, mChar ));

	std::string bpSectNumber	= JS_GetStringBytes( JS_ValueToString( cx, argv[2] ) );
	bool bInPack				= true;
	UI16 iAmount				= 1;
	ObjectType itemType			= OT_ITEM;

	if( argc > 3 )
		iAmount					= static_cast< UI16 >(JSVAL_TO_INT( argv[3] ));
	if( argc > 4 )
	{
		std::string objType		= JS_GetStringBytes( JS_ValueToString( cx, argv[4] ) );
		itemType				= FindObjTypeFromString( objType );
	}
	if( argc > 5 )
		bInPack					= ( JSVAL_TO_BOOLEAN( argv[5] ) == JS_TRUE );

	CItem *newItem = Items->CreateScriptItem( mySock, myChar, bpSectNumber, iAmount, itemType, bInPack );
	if( newItem != NULL )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, newItem, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CreateBlankItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Creates a "blank" item with default values from client's tiledata
//|	Notes		-	Default values can be overridden through harditems.dfn
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CreateBlankItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 8 )
	{
		DoSEErrorMessage( "CreateBlankItem: Invalid number of arguments (takes 7)" );
		return JS_FALSE;
	}
	CItem *newItem			= NULL;
	CSocket *mySock			= NULL;
	if( argv[0] != JSVAL_NULL )
	{
		JSObject *mSock		= JSVAL_TO_OBJECT( argv[0] );
		mySock				= static_cast<CSocket *>(JS_GetPrivate( cx, mSock ));
	}

	JSObject *mChar			= JSVAL_TO_OBJECT( argv[1] );
	CChar *myChar			= static_cast<CChar *>(JS_GetPrivate( cx, mChar ));
	SI32 amount				= (SI32)JSVAL_TO_INT( argv[2] );
	std::string itemName	= JS_GetStringBytes( JS_ValueToString( cx, argv[3] ) );
	bool isString			= false; //Never used!!
	std::string szItemName;			 //Never used!!
	UI16 itemID				= INVALIDID;
	if( JSVAL_IS_STRING( argv[4] ) )
	{
		szItemName = JS_GetStringBytes( JS_ValueToString( cx, argv[4] ) ); //Never used!!
		isString = true; //Never used!!
	}
	else
		itemID				= (UI16)JSVAL_TO_INT( argv[4] );
	UI16 colour				= (UI16)JSVAL_TO_INT( argv[5] );
	std::string objType		= JS_GetStringBytes( JS_ValueToString( cx, argv[6] ) );
	ObjectType itemType		= FindObjTypeFromString( objType );
	bool inPack				= ( JSVAL_TO_BOOLEAN( argv[7] ) == JS_TRUE );

	newItem = Items->CreateItem( mySock, myChar, itemID, amount, colour, itemType, inPack );
	if( newItem != NULL )
	{
		newItem->SetName( itemName );
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, newItem, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetMurderThreshold( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the max amount of kills allowed before a player turns red
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetMurderThreshold( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->RepMaxKills() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RollDice( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Rolls a die with specified number of sides, and adds a fixed value
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RollDice( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 3 )
	{
		DoSEErrorMessage( "RollDice: Invalid number of arguments (takes 3)" );
		return JS_FALSE;
	}
	UI32 numDice = JSVAL_TO_INT( argv[0] );
	UI32 numFace = JSVAL_TO_INT( argv[1] );
	UI32 numAdd  = JSVAL_TO_INT( argv[2] );

	cDice toRoll( numDice, numFace, numAdd );

	*rval = INT_TO_JSVAL( toRoll.roll() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_RaceCompareByRace( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Compares the relations between two races
//|	Notes		-	0 - neutral
//|					1 to 100 - allies
//|					-1 to -100 - enemies
//o-----------------------------------------------------------------------------------------------o
JSBool SE_RaceCompareByRace( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		return JS_FALSE;
	}
	RACEID r0 = (RACEID)JSVAL_TO_INT( argv[0] );
	RACEID r1 = (RACEID)JSVAL_TO_INT( argv[1] );
	*rval = INT_TO_JSVAL( Races->CompareByRace( r0, r1 ) );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_FindMulti( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns multi at given coordinates, world and instanceID
//o-----------------------------------------------------------------------------------------------o
JSBool SE_FindMulti( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 && argc != 4 && argc != 5 )
	{
		DoSEErrorMessage( "FindMulti: Invalid number of parameters (1, 4 or 5)" );
		return JS_FALSE;
	}
	SI16 xLoc = 0, yLoc = 0;
	SI08 zLoc = 0;
	UI08 worldNumber = 0;
	UI16 instanceID = 0;
	if( argc == 1 )
	{
		JSObject *myitemptr = JSVAL_TO_OBJECT( argv[0] );
		CBaseObject *myItemPtr = static_cast<CBaseObject *>(JS_GetPrivate( cx, myitemptr ));
		if( ValidateObject( myItemPtr ) )
		{
			xLoc		= myItemPtr->GetX();
			yLoc		= myItemPtr->GetY();
			zLoc		= myItemPtr->GetZ();
			worldNumber = myItemPtr->WorldNumber();
			instanceID	= myItemPtr->GetInstanceID();
		}
		else
		{
			DoSEErrorMessage( "FindMulti: Invalid object type" );
			return JS_FALSE;
		}
	}
	else
	{
		xLoc		= (SI16)JSVAL_TO_INT( argv[0] );
		yLoc		= (SI16)JSVAL_TO_INT( argv[1] );
		zLoc		= (SI08)JSVAL_TO_INT( argv[2] );
		worldNumber = (UI08)JSVAL_TO_INT( argv[3] );
		if( argc == 5 )
		{
			instanceID = (UI16)JSVAL_TO_INT( argv[4] );
		}
	}
	CMultiObj *multi = findMulti( xLoc, yLoc, zLoc, worldNumber, instanceID );
	if( ValidateObject( multi ) )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, multi, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	12 February, 2006
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns item closest to specified coordinates
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 4 && argc != 5 )
	{
		DoSEErrorMessage( "GetItem: Invalid number of parameters (4 or 5)" );
		return JS_FALSE;
	}
	SI16 xLoc = 0, yLoc = 0;
	SI08 zLoc = 0;
	UI08 worldNumber = 0;
	UI16 instanceID = 0;

	xLoc		= (SI16)JSVAL_TO_INT( argv[0] );
	yLoc		= (SI16)JSVAL_TO_INT( argv[1] );
	zLoc		= (SI08)JSVAL_TO_INT( argv[2] );
	worldNumber = (UI08)JSVAL_TO_INT( argv[3] );
	if( argc == 5 )
	{
		instanceID = (UI16)JSVAL_TO_INT( argv[4] );
	}

	CItem *item = GetItemAtXYZ( xLoc, yLoc, zLoc, worldNumber, instanceID );
	if( ValidateObject( item ) )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, item, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_FindItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	12 February, 2006
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns item of given ID that is closest to specified coordinates
//o-----------------------------------------------------------------------------------------------o
JSBool SE_FindItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 5 && argc != 6 )
	{
		DoSEErrorMessage( "FindItem: Invalid number of parameters (5 or 6)" );
		return JS_FALSE;
	}
	SI16 xLoc = 0, yLoc = 0;
	SI08 zLoc = 0;
	UI08 worldNumber = 0;
	UI16 id = 0;
	UI16 instanceID = 0;

	xLoc		= (SI16)JSVAL_TO_INT( argv[0] );
	yLoc		= (SI16)JSVAL_TO_INT( argv[1] );
	zLoc		= (SI08)JSVAL_TO_INT( argv[2] );
	worldNumber = (UI08)JSVAL_TO_INT( argv[3] );
	id			= (UI16)JSVAL_TO_INT( argv[4] );
	if( argc == 6 )
	{
		instanceID = (UI16)JSVAL_TO_INT( argv[5] );
	}

	CItem *item = FindItemNearXYZ( xLoc, yLoc, zLoc, worldNumber, id, instanceID );
	if( ValidateObject( item ) )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, item, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CompareGuildByGuild( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Compares the relations between two guilds
//|	Notes		-	0 - Neutral
//|					1 - War
//|					2 - Ally
//|					3 - Unknown
//|					4 - Same
//|					5 - Count
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CompareGuildByGuild( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		return JS_FALSE;
	}
	GUILDID toCheck		= (GUILDID)JSVAL_TO_INT( argv[0] );
	GUILDID toCheck2	= (GUILDID)JSVAL_TO_INT( argv[1] );
	*rval = INT_TO_JSVAL( GuildSys->Compare( toCheck, toCheck2 ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_PossessTown( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Source town takes control over target town
//o-----------------------------------------------------------------------------------------------o
JSBool SE_PossessTown( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		return JS_FALSE;
	}
	UI16 town	= (UI16)JSVAL_TO_INT( argv[0] );
	UI16 sTown	= (UI16)JSVAL_TO_INT( argv[1] );
	cwmWorldState->townRegions[town]->Possess( cwmWorldState->townRegions[sTown] );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_IsRaceWeakToWeather( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Checks if character's race is affected by given type of weather
//o-----------------------------------------------------------------------------------------------o
JSBool SE_IsRaceWeakToWeather( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		return JS_FALSE;
	}
	RACEID race		= (RACEID)JSVAL_TO_INT( argv[0] );
	weathID toCheck = (weathID)JSVAL_TO_INT( argv[1] );
	CRace *tRace	= Races->Race( race );
	if( tRace == NULL || toCheck >= WEATHNUM )
	{
		return JS_FALSE;
	}
	*rval = BOOLEAN_TO_JSVAL( tRace->AffectedBy( (WeatherType)toCheck ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetRaceSkillAdjustment( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns damage modifier for specified skill based on race
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetRaceSkillAdjustment( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		return JS_FALSE;
	}
	RACEID race = (RACEID)JSVAL_TO_INT( argv[0] );
	SI32 skill = JSVAL_TO_INT( argv[1] );
	*rval = INT_TO_JSVAL( Races->DamageFromSkill( skill, race ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_UseDoor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Uses specified door
//o-----------------------------------------------------------------------------------------------o
JSBool SE_UseDoor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "UseDoor: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}
	JSObject *mSock	= JSVAL_TO_OBJECT( argv[0] );
	JSObject *mDoor	= JSVAL_TO_OBJECT( argv[1] );

	CSocket *mySock	= static_cast<CSocket *>(JS_GetPrivate( cx, mSock ));
	CItem *myDoor	= static_cast<CItem *>(JS_GetPrivate( cx, mDoor ));

	if( !ValidateObject( myDoor ) )
	{
		DoSEErrorMessage( "UseDoor: Invalid door" );
		return JS_FALSE;
	}

	CChar *mChar = mySock->CurrcharObj();
	if( !ValidateObject( mChar ) )
	{
		DoSEErrorMessage( "UseDoor: Invalid character" );
		return JS_FALSE;
	}

	if( JSMapping->GetEnvokeByType()->Check( static_cast<UI16>(myDoor->GetType()) ) )
	{
		UI16 envTrig = JSMapping->GetEnvokeByType()->GetScript( static_cast<UI16>(myDoor->GetType()) );
		cScript *envExecute = JSMapping->GetScript( envTrig );
		if( envExecute != NULL )
			envExecute->OnUseChecked( mChar, myDoor );
	}

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_TriggerEvent( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Triggers a an event/function in a different JS
//|	Notes		-	Takes at least 2 parameters, which is the script number to trigger and the
//|					function name to call. Any extra parameters are extra parameters to the JS event
//o-----------------------------------------------------------------------------------------------o
JSBool SE_TriggerEvent( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 2 )
	{
		return JS_FALSE;
	}

	UI16 scriptNumberToFire = (UI16)JSVAL_TO_INT( argv[0] );
	char *eventToFire		= JS_GetStringBytes( JS_ValueToString( cx, argv[1] ) );
	cScript *toExecute		= JSMapping->GetScript( scriptNumberToFire );

	if( toExecute == NULL || eventToFire == NULL )
		return JS_FALSE;

	if( toExecute->CallParticularEvent( eventToFire, &argv[2], argc - 2 ) )
	{
		*rval = JS_TRUE;
		return JS_TRUE;
	}
	else
	{
		*rval = JS_FALSE;
		return JS_FALSE;
	}
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetPackOwner( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns owner of container item is contained in (if any)
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetPackOwner( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 2 )
	{
		DoSEErrorMessage( "GetPackOwner: Invalid number of arguments (takes 2)" );
		return JS_FALSE;
	}

	UI08 mType		= (UI08)JSVAL_TO_INT( argv[1] );
	CChar *pOwner	= NULL;

	if( mType == 0 )	// item
	{
		JSObject *mItem	= JSVAL_TO_OBJECT( argv[0] );
		CItem *myItem	= static_cast<CItem *>(JS_GetPrivate( cx, mItem ));
		pOwner			= FindItemOwner( myItem );
	}
	else				// serial
	{
		SI32 mSerItem	= JSVAL_TO_INT( argv[0] );
		pOwner			= FindItemOwner( calcItemObjFromSer( mSerItem ) );
	}
	if( ValidateObject( pOwner ) )
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_CHAR, pOwner, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	else
		*rval = JSVAL_NULL;
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CalcTargetedItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns targeted item stored on socket
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CalcTargetedItem( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "CalcTargetedItem: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}

	JSObject *mysockptr = JSVAL_TO_OBJECT( argv[0] );
	CSocket *sChar = static_cast<CSocket *>(JS_GetPrivate( cx, mysockptr ));
	if( sChar == NULL )
	{
		DoSEErrorMessage( "CalcTargetedItem: Invalid socket" );
		return JS_FALSE;
	}

	CItem *calcedItem = calcItemObjFromSer( sChar->GetDWord( 7 ) );
	if( !ValidateObject( calcedItem ) )
		*rval = JSVAL_NULL;
	else
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_ITEM, calcedItem, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CalcTargetedChar( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns targeted character stored on socket
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CalcTargetedChar( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "CalcTargetedChar: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}

	JSObject *mysockptr = JSVAL_TO_OBJECT( argv[0] );
	CSocket *sChar		= static_cast<CSocket *>(JS_GetPrivate( cx, mysockptr ));
	if( sChar == NULL )
	{
		DoSEErrorMessage( "CalcTargetedChar: Invalid socket" );
		return JS_FALSE;
	}

	CChar *calcedChar = calcCharObjFromSer( sChar->GetDWord( 7 ) );
	if( !ValidateObject( calcedChar ) )
		*rval = JSVAL_NULL;
	else
	{
		JSObject *myObj		= JSEngine->AcquireObject( IUE_CHAR, calcedChar, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
		*rval = OBJECT_TO_JSVAL( myObj );
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetTileIDAtMapCoord( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets the map tile ID at given coordinates
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetTileIDAtMapCoord( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 3 )
	{
		DoSEErrorMessage( "GetTileIDAtMapCoord: Invalid number of arguments (takes 3)" );
		return JS_FALSE;
	}

	UI16 xLoc			= (UI16)JSVAL_TO_INT( argv[0] );
	UI16 yLoc			= (UI16)JSVAL_TO_INT( argv[1] );
	UI08 wrldNumber		= (UI08)JSVAL_TO_INT( argv[2] );
	const map_st mMap	= Map->SeekMap( xLoc, yLoc, wrldNumber );
	*rval				= INT_TO_JSVAL( mMap.id );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_StaticInRange( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	17th August, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Checks for static within specified range of given location
//o-----------------------------------------------------------------------------------------------o
JSBool SE_StaticInRange( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 5 )
	{
		DoSEErrorMessage( "StaticInRange: Invalid number of arguments (takes 5, x, y, world, radius, tile)" );
		return JS_FALSE;
	}

	UI16 xLoc		= (UI16)JSVAL_TO_INT( argv[0] );
	UI16 yLoc		= (UI16)JSVAL_TO_INT( argv[1] );
	UI08 wrldNumber = (UI08)JSVAL_TO_INT( argv[2] );
	UI16 radius		= (UI16)JSVAL_TO_INT( argv[3] );
	UI16 tileID		= (UI16)JSVAL_TO_INT( argv[4] );
	bool tileFound	= false;

	for( SI32 i = xLoc - radius; i <= (xLoc + radius); ++i )
	{
		for( SI32 j = yLoc - radius; j <= (yLoc + radius); ++j )
		{
			CStaticIterator msi( xLoc, yLoc, wrldNumber );
			for( Static_st *mRec = msi.First(); mRec != NULL; mRec = msi.Next() )
			{
				if( mRec != NULL && mRec->itemid == tileID )
				{
					tileFound = true;
					break;
				}
			}
		}
	}

	*rval			= BOOLEAN_TO_JSVAL( tileFound );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_StaticAt( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	17th August, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Checks for static at specified location
//|	Notes		-	tile argument is optional; if not specified, will match ANY static found at location
//o-----------------------------------------------------------------------------------------------o
JSBool SE_StaticAt( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 4 && argc != 3 )
	{
		DoSEErrorMessage( "StaticAt: Invalid number of arguments (takes 4, x, y, world, tile)" );
		return JS_FALSE;
	}

	UI16 xLoc		= (UI16)JSVAL_TO_INT( argv[0] );
	UI16 yLoc		= (UI16)JSVAL_TO_INT( argv[1] );
	UI08 wrldNumber = (UI08)JSVAL_TO_INT( argv[2] );
	UI16 tileID		= 0xFFFF;
	bool tileMatch	= false;
	if( argc == 4 )
	{
		tileID		= (UI16)JSVAL_TO_INT( argv[3] );
		tileMatch	= true;
	}
	bool tileFound	= false;

	CStaticIterator msi( xLoc, yLoc, wrldNumber );
	for( Static_st *mRec = msi.First(); mRec != NULL; mRec = msi.Next() )
	{
		if( mRec != NULL && (!tileMatch || mRec->itemid==tileID ) )
		{
			tileFound = true;
			break;
		}
	}
	*rval			= BOOLEAN_TO_JSVAL( tileFound );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_StringToNum( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	27th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Converts string to number
//o-----------------------------------------------------------------------------------------------o
JSBool SE_StringToNum( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "StringToNum: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}

	UString str = JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );

	*rval = INT_TO_JSVAL( str.toInt() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_NumToString( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	27th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Converts number to string
//o-----------------------------------------------------------------------------------------------o
JSBool SE_NumToString( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "NumToString: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}

	SI32 num = JSVAL_TO_INT( argv[0] );
	auto str = str_number( num );
	*rval = STRING_TO_JSVAL( JS_NewStringCopyZ( cx, str.c_str() ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_NumToHexString( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	27th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Converts number to hex string
//o-----------------------------------------------------------------------------------------------o
JSBool SE_NumToHexString( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "NumToHexString: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}

	SI32 num = JSVAL_TO_INT( argv[0] );
	auto str = str_number( num, 16 );

	*rval = STRING_TO_JSVAL( JS_NewStringCopyZ( cx, str.c_str() ) );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetRaceCount( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	November 9, 2001
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the total number of races found in the server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetRaceCount( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 0 )
	{
		DoSEErrorMessage( "GetRaceCount: Invalid number of arguments (takes 0)" );
		return JS_FALSE;
	}
	*rval = INT_TO_JSVAL( Races->Count() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_AreaCharacterFunction( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	January 27, 2003
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Using a passed in function name, executes a JS function on an area of characters
//o-----------------------------------------------------------------------------------------------o
JSBool SE_AreaCharacterFunction( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 3 && argc != 4 )
	{
		// function name, source character, range
		DoSEErrorMessage( "AreaCharacterFunction: Invalid number of arguments (takes 3/4, function name, source character, range, optional socket)" );
		return JS_FALSE;
	}

	// Do parameter validation here
	JSObject *srcSocketObj		= NULL;
	CSocket *srcSocket			= NULL;
	char *trgFunc				= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	if( trgFunc == NULL )
	{
		DoSEErrorMessage( "AreaCharacterFunction: Argument 0 not a valid string" );
		return JS_FALSE;
	}

	JSObject *srcBaseObj	= JSVAL_TO_OBJECT( argv[1] );
	CBaseObject *srcObject		= static_cast<CBaseObject *>(JS_GetPrivate( cx, srcBaseObj ));

	if( !ValidateObject( srcObject ) )
	{
		DoSEErrorMessage( "AreaCharacterFunction: Argument 1 not a valid object" );
		return JS_FALSE;
	}
	R32 distance = static_cast<R32>(JSVAL_TO_INT( argv[2] ));
	if( argc == 4 )
	{
		srcSocketObj	= JSVAL_TO_OBJECT( argv[3] );
		srcSocket		= static_cast<CSocket *>(JS_GetPrivate( cx, srcSocketObj ));
	}

	UI16 retCounter				= 0;
	cScript *myScript			= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
	REGIONLIST nearbyRegions	= MapRegion->PopulateList( srcObject );
	for( REGIONLIST_CITERATOR rIter = nearbyRegions.begin(); rIter != nearbyRegions.end(); ++rIter )
	{
		CMapRegion *MapArea = (*rIter);
		if( MapArea == NULL )	// no valid region
			continue;
		CDataList< CChar * > *regChars = MapArea->GetCharList();
		regChars->Push();
		for( CChar *tempChar = regChars->First(); !regChars->Finished(); tempChar = regChars->Next() )
		{
			if( !ValidateObject( tempChar ) )
				continue;
			if( objInRange( srcObject, tempChar, (UI16)distance ) )
			{
				if( myScript->AreaObjFunc( trgFunc, srcObject, tempChar, srcSocket ) )
					++retCounter;
			}
		}
		regChars->Pop();
	}
	*rval = INT_TO_JSVAL( retCounter );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_AreaItemFunction( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	17th August, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Using a passed in function name, executes a JS function on an area of items
//o-----------------------------------------------------------------------------------------------o
JSBool SE_AreaItemFunction( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 3 && argc != 4 )
	{
		// function name, source character, range
		DoSEErrorMessage( "AreaItemFunction: Invalid number of arguments (takes 3/4, function name, source character, range, optional socket)" );
		return JS_FALSE;
	}

	// Do parameter validation here
	JSObject *srcSocketObj		= NULL;
	CSocket *srcSocket			= NULL;
	char *trgFunc				= JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	if( trgFunc == NULL )
	{
		DoSEErrorMessage( "AreaItemFunction: Argument 0 not a valid string" );
		return JS_FALSE;
	}


	JSObject *srcBaseObj	= JSVAL_TO_OBJECT( argv[1] );
	CBaseObject *srcObject		= static_cast<CBaseObject *>(JS_GetPrivate( cx, srcBaseObj ));

	if( !ValidateObject( srcObject ) )
	{
		DoSEErrorMessage( "AreaItemFunction: Argument 1 not a valid object" );
		return JS_FALSE;
	}
	R32 distance = static_cast<R32>(JSVAL_TO_INT( argv[2] ));
	if( argc == 4 )
	{
		srcSocketObj	= JSVAL_TO_OBJECT( argv[3] );
		srcSocket		= static_cast<CSocket *>(JS_GetPrivate( cx, srcSocketObj ));
	}

	UI16 retCounter					= 0;
	cScript *myScript				= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
	REGIONLIST nearbyRegions		= MapRegion->PopulateList( srcObject );
	for( REGIONLIST_CITERATOR rIter = nearbyRegions.begin(); rIter != nearbyRegions.end(); ++rIter )
	{
		CMapRegion *MapArea = (*rIter);
		if( MapArea == NULL )	// no valid region
			continue;
		CDataList< CItem * > *regItems = MapArea->GetItemList();
		regItems->Push();
		for( CItem *tempItem = regItems->First(); !regItems->Finished(); tempItem = regItems->Next() )
		{
			if( !ValidateObject( tempItem ) )
				continue;
			if( objInRange( srcObject, tempItem, (UI16)distance ) )
			{
				if( myScript->AreaObjFunc( trgFunc, srcObject, tempItem, srcSocket ) )
					++retCounter;
			}
		}
		regItems->Pop();
	}
	*rval = INT_TO_JSVAL( retCounter );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetDictionaryEntry( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	7/17/2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Allows the JSScripts to pull entries from the dictionaries and convert them to a string.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetDictionaryEntry( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 1 )
	{
		DoSEErrorMessage( "GetDictionaryEntry: Invalid number of arguments (takes at least 1)" );
		return JS_FALSE;
	}

	SI32 dictEntry = (SI32)JSVAL_TO_INT( argv[0] );
	UnicodeTypes language = ZERO;
	if( argc == 2 )
		language = (UnicodeTypes)JSVAL_TO_INT( argv[1] );
	std::string txt = Dictionary->GetEntry( dictEntry, language );

	JSString *strTxt = NULL;
	strTxt = JS_NewStringCopyZ( cx, txt.c_str() );
	*rval = STRING_TO_JSVAL( strTxt );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_Yell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	7/17/2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Globally yell a message from JS (Based on Commandlevel)
//o-----------------------------------------------------------------------------------------------o
JSBool SE_Yell( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc < 3 )
	{
		DoSEErrorMessage( "Yell: Invalid number of arguments (takes 3)" );
		return JS_FALSE;
	}

	JSObject *mSock			= JSVAL_TO_OBJECT( argv[0] );
	CSocket *mySock			= static_cast<CSocket *>(JS_GetPrivate( cx, mSock ));
	CChar *myChar			= mySock->CurrcharObj();
	std::string textToYell	= JS_GetStringBytes( JS_ValueToString( cx, argv[1] ) );
	UI08 commandLevel		= (UI08)JSVAL_TO_INT( argv[2] );

	std::string yellTo = "";
	switch( (CommandLevels)commandLevel )
	{
		case CL_PLAYER:			yellTo = " (GLOBAL YELL): ";	break;
		case CL_CNS:			yellTo = " (CNS YELL): ";		break;
		case CL_GM:				yellTo = " (GM YELL): ";		break;
		case CL_ADMIN:			yellTo = " (ADMIN YELL): ";		break;
	}

	UString tmpString = myChar->GetName() + yellTo + textToYell;

	CSpeechEntry& toAdd = SpeechSys->Add();
	toAdd.Speech( tmpString );
	toAdd.Font( (FontType)myChar->GetFontType() );
	toAdd.Speaker( INVALIDSERIAL );
	if( mySock->GetWord( 4 ) == 0x1700 )
		toAdd.Colour( 0x5A );
	else if( mySock->GetWord( 4 ) == 0x0 )
		toAdd.Colour( 0x5A );
	else
		toAdd.Colour( mySock->GetWord( 4 ) );
	toAdd.Type( SYSTEM );
	toAdd.At( cwmWorldState->GetUICurrentTime() );
	toAdd.TargType( SPTRG_BROADCASTALL );
	toAdd.CmdLevel( commandLevel );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_Reload( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	29 Dec 2003
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Reloads certain server subsystems
//o-----------------------------------------------------------------------------------------------o
JSBool SE_Reload( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "Reload: needs 1 argument!" );
		return JS_FALSE;
	}

	SI32 toLoad = JSVAL_TO_INT( argv[0] );

	switch( toLoad )
	{
		case 0:	// Reload regions
			UnloadRegions();
			LoadRegions();
			LoadTeleportLocations();
			break;
		case 1:	// Reload spawn regions
			UnloadSpawnRegions();
			LoadSpawnRegions();
			break;
		case 2:	// Reload Spells
			Magic->LoadScript();
			break;
		case 3: // Reload Commands
			Commands->Load();
			break;
		case 4:	// Reload DFNs
			FileLookup->Reload();
			LoadSkills();
			Skills->Load();
			break;
		case 5: // Reload JScripts
			messageLoop << MSG_RELOADJS;
			break;
		case 6: // Reload HTMLTemplates
			HTMLTemplates->Unload();
			HTMLTemplates->Load();
			break;
		case 7:	// Reload INI
			cwmWorldState->ServerData()->Load();
			break;
		case 8: // Reload Everything
			FileLookup->Reload();
			UnloadRegions();
			LoadRegions();
			UnloadSpawnRegions();
			LoadSpawnRegions();
			Magic->LoadScript();
			Commands->Load();
			LoadSkills();
			Skills->Load();
			messageLoop << MSG_RELOADJS;
			HTMLTemplates->Unload();
			HTMLTemplates->Load();
			cwmWorldState->ServerData()->Load();
			break;
		case 9: // Reload Accounts
			Accounts->Load();
			break;
		default:
			break;
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_SendStaticStats( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	25th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Builds an info gump for specified static or map tile
//o-----------------------------------------------------------------------------------------------o
JSBool SE_SendStaticStats( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "SendStaticStats: needs 1 argument!" );
		return JS_FALSE;
	}

	JSObject *mSock			= JSVAL_TO_OBJECT( argv[0] );
	CSocket *mySock			= static_cast<CSocket *>(JS_GetPrivate( cx, mSock ));
	if( mySock == NULL )
	{
		DoSEErrorMessage( "SendStaticStats: passed an invalid socket!" );
		return JS_FALSE;
	}
	CChar *myChar			= mySock->CurrcharObj();
	if( !ValidateObject( myChar ) )
		return JS_TRUE;

	if( mySock->GetDWord( 7 ) == 0 )
	{
		UI08 worldNumber	= myChar->WorldNumber();
		UI16 targetID		= mySock->GetWord( 0x11 );
		SI16 targetX		= mySock->GetWord( 0x0B );		// store our target x y and z locations
		SI16 targetY		= mySock->GetWord( 0x0D );
		SI08 targetZ		= mySock->GetByte( 0x10 );
		if( targetID != 0 )	// we might have a static rock or mountain
		{
			CStaticIterator msi( targetX, targetY, worldNumber );
			CMulHandler tileXTemp;
			if( cwmWorldState->ServerData()->ServerUsingHSTiles() )
			{
				//7.0.9.2 tiledata and later
				for( Static_st *stat = msi.First(); stat != NULL; stat = msi.Next() )
				{
					CTileHS& tile = Map->SeekTileHS( stat->itemid );
					if( targetZ == stat->zoff )
					{
						GumpDisplay staticStat( mySock, 300, 300 );
						staticStat.SetTitle( "Item [Static]" );
						staticStat.AddData( "ID", targetID, 5 );
						staticStat.AddData( "Height", tile.Height() );
						staticStat.AddData( "Name", tile.Name() );
						staticStat.Send( 4, false, INVALIDSERIAL );
					}
				}
			}
			else
			{
				//7.0.8.2 tiledata and earlier
				for( Static_st *stat = msi.First(); stat != NULL; stat = msi.Next() )
				{
					CTile& tile = Map->SeekTile( stat->itemid );
					if( targetZ == stat->zoff )
					{
						GumpDisplay staticStat( mySock, 300, 300 );
						staticStat.SetTitle( "Item [Static]" );
						staticStat.AddData( "ID", targetID, 5 );
						staticStat.AddData( "Height", tile.Height() );
						staticStat.AddData( "Name", tile.Name() );
						staticStat.Send( 4, false, INVALIDSERIAL );
					}
				}
			}
		}
		else		// or it could be a map only
		{
			// manually calculating the ID's if a maptype
			const map_st map1 = Map->SeekMap( targetX, targetY, worldNumber );
			GumpDisplay mapStat( mySock, 300, 300 );
			mapStat.SetTitle( "Item [Map]" );
			mapStat.AddData( "ID", targetID, 5 );
			if( cwmWorldState->ServerData()->ServerUsingHSTiles() )
			{
				//7.0.9.0 data and later
				CLandHS& land = Map->SeekLandHS( map1.id );
				mapStat.AddData( "Name", land.Name() );
			}
			else
			{
				//7.0.8.2 data and earlier
				CLand& land = Map->SeekLand( map1.id );
				mapStat.AddData( "Name", land.Name() );
			}
			mapStat.Send( 4, false, INVALIDSERIAL );
		}
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetTileHeight( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets the tile height of a specified tile (item)
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetTileHeight( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "GetTileHeight: needs 1 argument!" );
		return JS_FALSE;
	}

	UI16 tileNum = (UI16)JSVAL_TO_INT( argv[0] );
	*rval = INT_TO_JSVAL( Map->TileHeight( tileNum ) );
	return JS_TRUE;
}

bool SE_IterateFunctor( CBaseObject *a, UI32 &b, void *extraData )
{
	cScript *myScript = static_cast<cScript *>(extraData);
	return myScript->OnIterate( a, b );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_IterateOver( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	July 25th, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Loops through all world objects
//o-----------------------------------------------------------------------------------------------o
JSBool SE_IterateOver( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "IterateOver: needs 1 argument!" );
		return JS_FALSE;
	}

	UI32 b				= 0;
	std::string objType = JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) );
	ObjectType toCheck	= FindObjTypeFromString( objType );
	cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
	if( myScript != NULL )
		ObjectFactory::getSingleton().IterateOver( toCheck, b, myScript, &SE_IterateFunctor );

	JS_MaybeGC( cx );

	*rval = INT_TO_JSVAL( b );
	return JS_TRUE;
}

bool SE_IterateSpawnRegionsFunctor( CSpawnRegion *a, UI32 &b, void *extraData )
{
	cScript *myScript = static_cast<cScript *>( extraData );
	return myScript->OnIterateSpawnRegions( a, b );
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_IterateOverSpawnRegions( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	July, 2020
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Loops over all spawn regions in the world
//o-----------------------------------------------------------------------------------------------o
JSBool SE_IterateOverSpawnRegions( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	UI32 b = 0;
	cScript *myScript = JSMapping->GetScript( JS_GetGlobalObject( cx ) );

	if( myScript != NULL )
	{
		SPAWNMAP_CITERATOR spIter = cwmWorldState->spawnRegions.begin();
		SPAWNMAP_CITERATOR spEnd = cwmWorldState->spawnRegions.end();
		while( spIter != spEnd )
		{
			CSpawnRegion *spawnReg = spIter->second;
			if( spawnReg != NULL )
			{
				SE_IterateSpawnRegionsFunctor( spawnReg, b, myScript );
			}
			++spIter;
		}
	}

	JS_MaybeGC( cx );

	*rval = INT_TO_JSVAL( b );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_WorldBrightLevel( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets and sets world bright level - brightest part of the regular day/night cycle
//o-----------------------------------------------------------------------------------------------o
JSBool SE_WorldBrightLevel( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 1 )
	{
		DoSEErrorMessage(format( "WorldBrightLevel: Unknown Count of Arguments: %d", argc) );
		return JS_FALSE;
	}
	else if( argc == 1 )
	{
		LIGHTLEVEL brightLevel = (LIGHTLEVEL)JSVAL_TO_INT( argv[0] );
		cwmWorldState->ServerData()->WorldLightBrightLevel( brightLevel );
	}
	*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->WorldLightBrightLevel() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_WorldDarkLevel( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets and sets world dark level - darkest part of the regular day/night cycle
//o-----------------------------------------------------------------------------------------------o
JSBool SE_WorldDarkLevel( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 1 )
	{
		DoSEErrorMessage( format("WorldDarkLevel: Unknown Count of Arguments: %d", argc) );
		return JS_FALSE;
	}
	else if( argc == 1 )
	{
		LIGHTLEVEL darkLevel = (LIGHTLEVEL)JSVAL_TO_INT( argv[0] );
		cwmWorldState->ServerData()->WorldLightDarkLevel( darkLevel );
	}
	*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->WorldLightDarkLevel() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_WorldDungeonLevel( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th July, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets and sets default light level in dungeons
//o-----------------------------------------------------------------------------------------------o
JSBool SE_WorldDungeonLevel( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 1 )
	{
		DoSEErrorMessage( format("WorldDungeonLevel: Unknown Count of Arguments: %d", argc) );
		return JS_FALSE;
	}
	else if( argc == 1 )
	{
		LIGHTLEVEL dungeonLevel = (LIGHTLEVEL)JSVAL_TO_INT( argv[0] );
		cwmWorldState->ServerData()->DungeonLightLevel( dungeonLevel );
	}
	*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->DungeonLightLevel() );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetSocketFromIndex( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	3rd August, 2004
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns socket based on provided index, from list of connected clients
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetSocketFromIndex( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "GetSocketFromIndex: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	UOXSOCKET index = (UOXSOCKET)JSVAL_TO_INT( argv[0] );

	CSocket *mSock	= Network->GetSockPtr( index );
	CChar *mChar	= NULL;
	if( mSock != NULL )
		mChar = mSock->CurrcharObj();

	if( !ValidateObject( mChar ) )
	{
		*rval = JSVAL_NULL;
		return JS_FALSE;
	}

	JSObject *myObj		= JSEngine->AcquireObject( IUE_CHAR, mChar, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
	*rval = OBJECT_TO_JSVAL( myObj );
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ReloadJSFile( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	5th February, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Reload specified JS file by scriptID
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ReloadJSFile( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "ReloadJSFile: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}
	UI16 scriptID			= static_cast<UI16>(JSVAL_TO_INT( argv[0] ));
	if( scriptID == JSMapping->GetScriptID( JS_GetGlobalObject( cx ) ) )
	{
		DoSEErrorMessage( format("ReloadJSFile: JS Script attempted to reload itself, crash avoided (ScriptID %u)", scriptID) );
		return JS_FALSE;
	}

	JSMapping->Reload( scriptID );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ResourceArea( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th September, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets/Sets amount of resource areas to split the world into
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ResourceArea( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 2 || argc == 0 )
	{
		DoSEErrorMessage( format("ResourceArea: Invalid Count of Arguments: %d", argc) );
		return JS_FALSE;
	}

	UString resType = UString( JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) ) ).stripWhiteSpace().upper();
	if( argc == 2 )
	{
		UI16 newVal = static_cast<UI16>(JSVAL_TO_INT( argv[1] ));
		if( resType == "LOGS" )	// Logs
			cwmWorldState->ServerData()->ResLogArea( newVal );
		else if( resType == "ORE" )	// Ore
			cwmWorldState->ServerData()->ResOreArea( newVal );
	}

	if( resType == "LOGS" )
		*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ResLogArea() );
	else if( resType == "ORE" )
		*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ResOreArea() );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ResourceAmount( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th September, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets/Sets amount of resources (logs/ore) in each resource area on the server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ResourceAmount( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 2 || argc == 0 )
	{
		DoSEErrorMessage( format("ResourceAmount: Invalid Count of Arguments: %d", argc) );
		return JS_FALSE;
	}

	UString resType = UString( JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) ) ).stripWhiteSpace().upper();
	if( argc == 2 )
	{
		SI16 newVal = static_cast<SI16>(JSVAL_TO_INT( argv[1] ));
		if( resType == "LOGS" )
			cwmWorldState->ServerData()->ResLogs( newVal );
		else if( resType == "ORE" )
			cwmWorldState->ServerData()->ResOre( newVal );
	}

	if( resType == "LOGS" )
		*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ResLogs() );
	else if( resType == "ORE" )
		*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ResOre() );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ResourceTime( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th September, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets/Sets respawn timers for ore/log resources on server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ResourceTime( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 2 || argc == 0 )
	{
		DoSEErrorMessage( format("ResourceTime: Invalid Count of Arguments: %d", argc) );
		return JS_FALSE;
	}

	UString resType = UString( JS_GetStringBytes( JS_ValueToString( cx, argv[0] ) ) ).stripWhiteSpace().upper();
	if( argc == 2 )
	{
		UI16 newVal = static_cast<UI16>(JSVAL_TO_INT( argv[1] ));
		if( resType == "LOGS" )
			cwmWorldState->ServerData()->ResLogTime( newVal );
		else if( resType == "ORE" )
			cwmWorldState->ServerData()->ResOreTime( newVal );
	}

	if( resType == "LOGS" )
		*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ResLogTime() );
	else if( resType == "ORE" )
		*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ResOreTime() );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ResourceRegion( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	18th September, 2005
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns a resource object allowing JS to modify resource data.
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ResourceRegion( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 3 )
	{
		DoSEErrorMessage( "ResourceRegion: Invalid number of arguments (takes 3)" );
		return JS_FALSE;
	}
	SI16 x			= static_cast<SI16>(JSVAL_TO_INT( argv[0] ));
	SI16 y			= static_cast<SI16>(JSVAL_TO_INT( argv[1] ));
	UI08 worldNum	= static_cast<UI08>(JSVAL_TO_INT( argv[2] ));
	MapResource_st *mRes = MapRegion->GetResource( x, y, worldNum );
	if( mRes == NULL )
	{
		DoSEErrorMessage( "ResourceRegion: Invalid Resource Region" );
		return JS_FALSE;
	}

	JSObject *jsResource = JS_NewObject( cx, &UOXResource_class, NULL, obj );
	JS_DefineProperties( cx, jsResource, CResourceProperties );
	JS_SetPrivate( cx, jsResource, mRes );

	*rval = OBJECT_TO_JSVAL( jsResource );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ValidateObject( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	26th January, 2006
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Checks if object is a valid and not NULL or marked for deletion
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ValidateObject( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "ValidateObject: Invalid number of arguments (takes 1)" );
		return JS_FALSE;
	}

	JSEncapsulate myClass( cx, &(argv[0]) );

	if( myClass.ClassName() == "UOXChar" || myClass.ClassName() == "UOXItem" )
	{
		CBaseObject *myObj	= static_cast<CBaseObject *>(myClass.toObject());
		*rval				= BOOLEAN_TO_JSVAL( ValidateObject( myObj ) );
	}
	else
		*rval = JSVAL_FALSE;

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ApplyDamageBonuses( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	17th March, 2006
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns damage bonuses based on race/weather weakness and character skills
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ApplyDamageBonuses( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 6 )
	{
		DoSEErrorMessage( "ApplyDamageBonuses: Invalid number of arguments (takes 6)" );
		return JS_FALSE;
	}

	CChar *attacker	= NULL, *defender = NULL;
	SI16 damage = 0;

	JSEncapsulate damageType( cx, &( argv[0] ) );
	JSEncapsulate getFightSkill( cx, &( argv[3] ) );
	JSEncapsulate hitLoc( cx, &( argv[4] ) );
	JSEncapsulate baseDamage( cx, &(argv[5]) );

	JSEncapsulate attackerClass( cx, &(argv[1]) );
	if( attackerClass.ClassName() != "UOXChar" )	// It must be a character!
	{
		DoSEErrorMessage( "ApplyDamageBonuses: Passed an invalid Character" );
		return JS_FALSE;
	}

	if( attackerClass.isType( JSOT_VOID ) || attackerClass.isType( JSOT_NULL ) )
	{
		DoSEErrorMessage( "ApplyDamageBonuses: Passed an invalid Character" );
		return JS_TRUE;
	}
	else
	{
		attacker	= static_cast<CChar *>(attackerClass.toObject());
		if( !ValidateObject( attacker )  )
		{
			DoSEErrorMessage( "ApplyDamageBonuses: Passed an invalid Character" );
			return JS_TRUE;
		}
	}

	JSEncapsulate defenderClass( cx, &(argv[2]) );
	if( defenderClass.ClassName() != "UOXChar" )	// It must be a character!
	{
		DoSEErrorMessage( "ApplyDamageBonuses: Passed an invalid Character" );
		return JS_FALSE;
	}

	if( defenderClass.isType( JSOT_VOID ) || defenderClass.isType( JSOT_NULL ) )
	{
		DoSEErrorMessage( "ApplyDamageBonuses: Passed an invalid Character" );
		return JS_TRUE;
	}
	else
	{
		defender	= static_cast<CChar *>(defenderClass.toObject());
		if( !ValidateObject( defender )  )
		{
			DoSEErrorMessage( "ApplyDamageBonuses: Passed an invalid Character" );
			return JS_TRUE;
		}
	}

	damage	= Combat->ApplyDamageBonuses( (WeatherType)damageType.toInt(), attacker, defender, (UI08)getFightSkill.toInt(), (UI08)hitLoc.toInt(), (SI16)baseDamage.toInt() );
	*rval	= INT_TO_JSVAL( damage );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_ApplyDefenseModifiers( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	17th March, 2006
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns defense modifiers based on shields/parrying, armor values and elemental damage
//o-----------------------------------------------------------------------------------------------o
JSBool SE_ApplyDefenseModifiers( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 7 )
	{
		DoSEErrorMessage( "ApplyDefenseModifiers: Invalid number of arguments (takes 7)" );
		return JS_FALSE;
	}

	CChar *attacker	= NULL, *defender = NULL;
	SI16 damage = 0;

	JSEncapsulate damageType( cx, &( argv[0] ) );
	JSEncapsulate getFightSkill( cx, &( argv[3] ) );
	JSEncapsulate hitLoc( cx, &( argv[4] ) );
	JSEncapsulate baseDamage( cx, &(argv[5]) );
	JSEncapsulate doArmorDamage(cx, &( argv[6] ) );

	JSEncapsulate attackerClass( cx, &(argv[1]) );
	if( attackerClass.ClassName() == "UOXChar" )
	{
		if( attackerClass.isType( JSOT_VOID ) || attackerClass.isType( JSOT_NULL ) )
		{
			attacker = NULL;
		}
		else
		{
			attacker	= static_cast<CChar *>(attackerClass.toObject());
			if( !ValidateObject( attacker )  )
			{
				attacker = NULL;
			}
		}
	}

	JSEncapsulate defenderClass( cx, &(argv[2]) );
	if( defenderClass.ClassName() != "UOXChar" )	// It must be a character!
	{
		DoSEErrorMessage( "ApplyDefenseModifiers: Passed an invalid Character" );
		return JS_FALSE;
	}

	if( defenderClass.isType( JSOT_VOID ) || defenderClass.isType( JSOT_NULL ) )
	{
		DoSEErrorMessage( "ApplyDefenseModifiers: Passed an invalid Character" );
		return JS_TRUE;
	}
	else
	{
		defender	= static_cast<CChar *>(defenderClass.toObject());
		if( !ValidateObject( defender )  )
		{
			DoSEErrorMessage( "ApplyDefenseModifiers: Passed an invalid Character" );
			return JS_TRUE;
		}
	}

	damage	= Combat->ApplyDefenseModifiers( (WeatherType)damageType.toInt(), attacker, defender, (UI08)getFightSkill.toInt(), (UI08)hitLoc.toInt(), (SI16)baseDamage.toInt(), doArmorDamage.toBool() );
	*rval	= INT_TO_JSVAL( damage );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_CreateParty( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	21st September, 2006
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Create a party with specified character as the party leader
//o-----------------------------------------------------------------------------------------------o
JSBool SE_CreateParty( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "CreateParty: Invalid number of arguments (takes 1, the leader)" );
		return JS_FALSE;
	}

	JSEncapsulate myClass( cx, &(argv[0]) );

	if( myClass.ClassName() == "UOXChar" || myClass.ClassName() == "UOXSocket" )
	{	// it's a character or socket, fantastic
		CChar *leader		= NULL;
		CSocket *leaderSock	= NULL;
		if( myClass.ClassName() == "UOXChar" )
		{
			leader		= static_cast<CChar *>(myClass.toObject());
			leaderSock	= leader->GetSocket();
		}
		else
		{
			leaderSock	= static_cast<CSocket *>(myClass.toObject());
			leader		= leaderSock->CurrcharObj();
		}

		if( PartyFactory::getSingleton().Get( leader ) != NULL )
		{
			*rval = JSVAL_NULL;
		}
		else
		{
			Party *tParty	= PartyFactory::getSingleton().Create( leader );
			JSObject *myObj	= JSEngine->AcquireObject( IUE_PARTY, tParty, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
			*rval			= OBJECT_TO_JSVAL( myObj );
		}
	}
	else	// anything else isn't a valid leader people
		*rval = JSVAL_NULL;

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_Moon( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	25th May, 2007
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Gets or sets Moon phases on server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_Moon( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc > 2 || argc == 0 )
	{
		DoSEErrorMessage( format("Moon: Invalid Count of Arguments: %d", argc) );
		return JS_FALSE;
	}

	SI16 slot = static_cast<SI16>(JSVAL_TO_INT( argv[0] ));
	if( argc == 2 )
	{
		SI16 newVal = static_cast<SI16>(JSVAL_TO_INT( argv[1] ));
		cwmWorldState->ServerData()->ServerMoon( slot, newVal );
	}

	*rval = INT_TO_JSVAL( cwmWorldState->ServerData()->ServerMoon( slot ) );

	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetSpawnRegion( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	June 22, 2020
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns a specified spawn region object
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetSpawnRegion( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 1 )
	{
		DoSEErrorMessage( "GetSpawnRegion: Invalid number of parameters (1)" );
		return JS_FALSE;
	}

	UI16 spawnRegNum = (UI16)JSVAL_TO_INT( argv[0] );
	if( cwmWorldState->spawnRegions.find( spawnRegNum ) != cwmWorldState->spawnRegions.end() )
	{
		CSpawnRegion *spawnReg = cwmWorldState->spawnRegions[spawnRegNum];
		if( spawnReg != NULL )
		{
			JSObject *myObj = JSEngine->AcquireObject( IUE_SPAWNREGION, spawnReg, JSEngine->FindActiveRuntime( JS_GetRuntime( cx ) ) );
			*rval = OBJECT_TO_JSVAL( myObj );
		}
		else
		{
			*rval = JSVAL_NULL;
		}
	}
	else
	{
		*rval = JSVAL_NULL;
	}
	return JS_TRUE;
}

//o-----------------------------------------------------------------------------------------------o
//|	Function	-	JSBool SE_GetSpawnRegionCount( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
//|	Date		-	June 22, 2020
//o-----------------------------------------------------------------------------------------------o
//|	Purpose		-	Returns the total number of spawn regions found in the server
//o-----------------------------------------------------------------------------------------------o
JSBool SE_GetSpawnRegionCount( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval )
{
	if( argc != 0 )
	{
		DoSEErrorMessage( "GetSpawnRegionCount: Invalid number of arguments (takes 0)" );
		return JS_FALSE;
	}
	*rval = INT_TO_JSVAL( cwmWorldState->spawnRegions.size() );
	return JS_TRUE;
}
