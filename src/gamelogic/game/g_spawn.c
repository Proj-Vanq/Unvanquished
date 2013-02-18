/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"
#include "g_spawn.h"

qboolean G_SpawnString( const char *key, const char *defaultString, char **out )
{
	int i;

	if ( !level.spawning )
	{
		*out = ( char * ) defaultString;
//    G_Error( "G_SpawnString() called while not spawning" );
		return qfalse;
	}

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		if ( !Q_stricmp( key, level.spawnVars[ i ][ 0 ] ) )
		{
			*out = level.spawnVars[ i ][ 1 ];
			return qtrue;
		}
	}

	*out = ( char * ) defaultString;
	return qfalse;
}

qboolean  G_SpawnFloat( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean G_SpawnInt( const char *key, const char *defaultString, int *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean  G_SpawnVector( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ] );
	return present;
}

qboolean  G_SpawnVector4( const char *key, const char *defaultString, float *out )
{
	char     *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f %f", &out[ 0 ], &out[ 1 ], &out[ 2 ], &out[ 3 ] );
	return present;
}

//
// fields are needed for spawning from the entity string
//
typedef enum
{
  F_INT,
  F_FLOAT,
  F_STRING,
  F_VECTOR,
  F_VECTOR4,
  F_YAW
} fieldtype_t;

typedef struct
{
	const char  *name;
	size_t      offset;
	fieldtype_t type;
	const int	versionState;
	const char  *replacement;
} field_t;

static const field_t fields[] =
{
	{ "acceleration",        FOFS( acceleration ),        F_VECTOR    },
	{ "alpha",               FOFS( pos1 ),                F_VECTOR    },
	{ "angle",               FOFS( s.angles ),            F_YAW       },
	{ "angles",              FOFS( s.angles ),            F_VECTOR    },
	{ "animation",           FOFS( animation ),           F_VECTOR4   },
	{ "bounce",              FOFS( physicsBounce ),       F_FLOAT     },
	{ "classname",           FOFS( classname ),           F_STRING    },
	{ "count",               FOFS( count ),               F_INT       },
	{ "dmg",                 FOFS( damage ),              F_INT       },
	{ "health",              FOFS( health ),              F_INT       },
	{ "message",             FOFS( message ),             F_STRING    },
	{ "model",               FOFS( model ),               F_STRING    },
	{ "model2",              FOFS( model2 ),              F_STRING    },
	{ "origin",              FOFS( s.origin ),            F_VECTOR    },
	{ "radius",              FOFS( pos2 ),                F_VECTOR    },
	{ "random",              FOFS( waitVariance ),        F_FLOAT,    ENT_V_TMPNAME, "waitVariance" },
	{ "spawnflags",          FOFS( spawnflags ),          F_INT       },
	{ "speed",               FOFS( speed ),               F_FLOAT     },
	{ "target",				 FOFS( targets[ 0 ] ),		  F_STRING	  },
	{ "target2", 			 FOFS( targets[ 1 ] ),		  F_STRING	  },
	{ "target3",			 FOFS( targets[ 2 ] ),		  F_STRING	  },
	{ "target4",			 FOFS( targets[ 3 ] ),		  F_STRING	  },
	{ "targetname",			 FOFS( targetnames[ 0 ] ),	  F_STRING	  },
	{ "targetname2",		 FOFS( targetnames[ 1 ] ),	  F_STRING	  },
	{ "targetname3",		 FOFS( targetnames[ 2 ] ),	  F_STRING	  },
	{ "targetname4",		 FOFS( targetnames[ 3 ] ),	  F_STRING	  },
	{ "targetShaderName",    FOFS( targetShaderName ),    F_STRING    },
	{ "targetShaderNewName", FOFS( targetShaderNewName ), F_STRING    },
	{ "wait",                FOFS( wait ),                F_FLOAT     },
	{ "waitVariance",        FOFS( waitVariance ),        F_FLOAT     },
};

typedef struct
{
	const char *name;
	void      ( *spawn )( gentity_t *ent );
	const int	versionState;
	const char  *replacement;
} spawn_t;

static const spawn_t spawns[] =
{
	/**
	 *
	 *	Environment entities
	 *	====================
	 *
	 */
	{ "env_lens_flare",           SP_env_lens_flare,          ENT_V_TMPNAME, "env_lens_flare"},
	{ "env_particle_system",      SP_env_particle_system      },
	{ "env_rumble",               SP_env_rumble               },
	{ "env_speaker",              SP_env_speaker              },

	/**
	 *
	 *	Flow entities
	 *	=============
	 *
	 */
	{ "flow_relay",               SP_flow_relay               },

	/**
	 *
	 *	Functional entities
	 *	====================
	 *
	 */
	{ "func_bobbing",             SP_func_bobbing             },
	{ "func_button",              SP_func_button              },
	{ "func_door",                SP_func_door                },
	{ "func_door_model",          SP_func_door_model          },
	{ "func_door_rotating",       SP_func_door_rotating       },
	{ "func_dynamic",             SP_func_dynamic             },
	{ "func_group",               SP_NULL                     },
	{ "func_pendulum",            SP_func_pendulum            },
	{ "func_plat",                SP_func_plat                },
	{ "func_rotating",            SP_func_rotating            },
	{ "func_static",              SP_func_static              },
	{ "func_timer",               SP_sensor_timer,            ENT_V_TMPNAME, "sensor_timer" },
	{ "func_train",               SP_func_train               },

	/**
	 *
	 *	Game entities
	 *	=============
	 *
	 */
	{ "game_score",               SP_game_score               },
	{ "game_end",                 SP_game_end                 },

	/**
	 *
	 *	Information entities
	 *	====================
	 *	info entities don't do anything at all, but provide positional
	 *	information for things controlled by other processes
	 *
	 */
	{ "info_alien_intermission",  SP_info_player_intermission },
	{ "info_human_intermission",  SP_info_player_intermission },
	{ "info_notnull",             SP_target_position,         ENT_V_TMPNAME, "target_position" },
	{ "info_null",                SP_NULL                     },
	{ "info_player_deathmatch",   SP_info_player_deathmatch   },
	{ "info_player_intermission", SP_info_player_intermission },
	{ "info_player_start",        SP_info_player_start        },
	{ "light",                    SP_NULL                     },
	{ "misc_anim_model",          SP_misc_anim_model          },
	{ "misc_light_flare",         SP_env_lens_flare,          ENT_V_TMPNAME, "env_lens_flare"},
	{ "misc_model",               SP_NULL                     },
	{ "misc_particle_system",     SP_env_particle_system,     ENT_V_TMPNAME, "env_particle_system"},
	{ "misc_portal_camera",       SP_misc_portal_camera       },
	{ "misc_portal_surface",      SP_misc_portal_surface      },
	{ "misc_teleporter_dest",     SP_target_position,         ENT_V_TMPNAME, "target_position" },
	{ "path_corner",              SP_path_corner              },

	/**
	 *  Sensors
	 *  =======
	 *  Sensor fire an event (usually towards targets) when aware
	 *  of another entity, event, or gamestate (timer and start being aware of the game start).
	 *  Sensors often can be targeted to toggle (activate or deactivate)
	 *  their function of perceiving other entities.
	 */

	{ "sensor_end",              SP_sensor_end                },
	{ "sensor_stage",            SP_sensor_stage              },
	{ "sensor_start",            SP_sensor_start              },
	{ "sensor_timer",            SP_sensor_timer              },
	{ "sensor_touch",            SP_sensor_touch              },

	/**
	 *
	 * 	Target Entities
	 * 	===============
	 * 	Targets perform no action by themselves.
	 *	Instead they are targeted by other entities,
	 *	like being triggered by a trigger_ entity.
	 *
	 */
	{ "target_alien_win",         SP_target_alien_win,        ENT_V_TMPNAME, "game_end" },
	{ "target_delay",             SP_flow_relay,              ENT_V_TMPNAME, "flow_relay" },
	{ "target_human_win",         SP_target_human_win,        ENT_V_TMPNAME, "game_end" },
	{ "target_hurt",              SP_target_hurt              },
	{ "target_kill",              SP_target_kill              },
	{ "target_location",          SP_target_location          },
	{ "target_position",          SP_target_position          },
	{ "target_print",             SP_target_print             },
	{ "target_push",              SP_target_push              },
	{ "target_relay",             SP_flow_relay,              ENT_V_TMPNAME, "flow_relay" },
	{ "target_rumble",            SP_env_rumble,              ENT_V_TMPNAME, "env_rumble" },
	{ "target_score",             SP_game_score,              ENT_V_TMPNAME, "game_score" },
	{ "target_speaker",           SP_env_speaker,             ENT_V_TMPNAME, "env_speaker" },
	{ "target_teleporter",        SP_target_teleporter        },
	{ "target_win",               SP_game_end,                ENT_V_TMPNAME, "game_end" },

	/**
	 *
	 *	Trigger
	 *	=======
	 *	Triggers cause a defined effect when aware of another entity, event, or gamestate.
	 *	In that sense it's like an integration of a sensor and a target
	 *	and might in some cases be modeled by a combination of them.
	 *
	 *	Triggers carry often the benefit of being predicted client-side
	 *	(since no entity chains have to be resolved first)
	 *	such as trigger_push and trigger_teleport.
	 *
	 */
	{ "trigger_always",           SP_sensor_start,            ENT_V_RENAMED, "sensor_start" },
	{ "trigger_ammo",             SP_trigger_ammo             },
	{ "trigger_buildable",        SP_sensor_touch_compat,     ENT_V_TMPNAME, "sensor_touch" },
	{ "trigger_class",            SP_sensor_touch_compat,     ENT_V_TMPNAME, "sensor_touch" },
	{ "trigger_equipment",        SP_sensor_touch_compat,     ENT_V_TMPNAME, "sensor_touch" },
	{ "trigger_gravity",          SP_trigger_gravity          },
	{ "trigger_heal",             SP_trigger_heal             },
	{ "trigger_hurt",             SP_trigger_hurt             },
	{ "trigger_multiple",         SP_trigger_multiple         },
	{ "trigger_push",             SP_trigger_push             },
	{ "trigger_stage",            SP_sensor_stage,            ENT_V_RENAMED, "sensor_stage" },
	{ "trigger_teleport",         SP_trigger_teleport         },
	{ "trigger_win",              SP_sensor_end,              ENT_V_TMPNAME, "sensor_end" }
};

qboolean G_HandleEntityVersions( spawn_t *spawnDescription, gentity_t *entity )
{
	if ( spawnDescription->versionState == ENT_V_CURRENT ) // we don't need to handle anything
		return qtrue;

	if ( !spawnDescription->replacement || !Q_stricmp(entity->classname, spawnDescription->replacement))
	{
		if ( g_debugEntities.integer > -2 )
			G_Printf( "^1ERROR: ^7entity ^5%s ^7has been marked deprecated but no replacement has been supplied\n", entity->classname );

		return qfalse;
	}

	if ( g_debugEntities.integer >= 0 ) //dont't warn about anything with -1 or lower
	{
		if( spawnDescription->versionState < ENT_V_TMPORARY
		|| ( g_debugEntities.integer >= 1 && spawnDescription->versionState >= ENT_V_TMPORARY) )
		{
			G_Printf( "^3WARNING: ^7deprecated entity classname ^5%s^7 found — use ^5%s^7 instead\n", entity->classname, spawnDescription->replacement );
		}
	}
	entity->classname = spawnDescription->replacement;
	return qtrue;
}

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent )
{
	spawn_t     *s;
	buildable_t buildable;

	if ( !ent->classname )
	{
		G_Printf( "G_CallSpawn: NULL classname\n" );
		return qfalse;
	}

	//check buildable spawn functions
	buildable = BG_BuildableByEntityName( ent->classname )->number;

	if ( buildable != BA_NONE )
	{
		// don't spawn built-in buildings if we are using a custom layout
		if ( level.layout[ 0 ] && Q_stricmp( level.layout, "*BUILTIN*" ) )
		{
			return qfalse;
		}

		if ( buildable == BA_A_SPAWN || buildable == BA_H_SPAWN )
		{
			ent->s.angles[ YAW ] += 180.0f;
			AngleNormalize360( ent->s.angles[ YAW ] );
		}

		G_SpawnBuildable( ent, buildable );
		return qtrue;
	}

	// check the spawn functions for other classes
	s = bsearch( ent->classname, spawns, ARRAY_LEN( spawns ),
	             sizeof( spawn_t ), cmdcmp );

	if ( s )
	{ // found it

		s->spawn( ent );

		/*
		 *  to allow each spawn function to test and handle for itself,
		 *  we handle it automatically *after* the spawn (but before it's use)
		 */
		return G_HandleEntityVersions( s, ent );
	}

	//don't even warn about spawning-errors with -2 (maps might still work at least partly if we ignore these willingly)
	if ( g_debugEntities.integer > -2 )
	{
		if (!Q_stricmp("worldspawn", ent->classname))
		{
			G_Printf( "^1ERROR: ^5%s ^7is not the first entity – we are unable to spawn it.\n", ent->classname );
		}
		else
		{
			G_Printf( "^1ERROR: ^5%s ^7doesn't have a spawn function. We have to skip it.\n", ent->classname );
		}
	}

	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string )
{
	char *newb, *new_p;
	int  i, l;

	l = strlen( string ) + 1;

	newb = BG_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i = 0; i < l; i++ )
	{
		if ( string[ i ] == '\\' && i < l - 1 )
		{
			i++;

			if ( string[ i ] == 'n' )
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '\\';
			}
		}
		else
		{
			*new_p++ = string[ i ];
		}
	}

	return newb;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *entity )
{
	field_t *resultingField;
	byte    *entityData;
	float   v;
	vec3_t  vec;
	vec4_t  vec4;

	resultingField = bsearch( key, fields, ARRAY_LEN( fields ),
	             sizeof( field_t ), cmdcmp );

	if ( !resultingField )
	{
		return;
	}

	entityData = ( byte * ) entity;

	switch ( resultingField->type )
	{
		case F_STRING:
			* ( char ** )( entityData + resultingField->offset ) = G_NewString( value );
			break;

		case F_VECTOR:
			sscanf( value, "%f %f %f", &vec[ 0 ], &vec[ 1 ], &vec[ 2 ] );

			( ( float * )( entityData + resultingField->offset ) ) [ 0 ] = vec[ 0 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 1 ] = vec[ 1 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 2 ] = vec[ 2 ];
			break;

		case F_VECTOR4:
			sscanf( value, "%f %f %f %f", &vec4[ 0 ], &vec4[ 1 ], &vec4[ 2 ], &vec4[ 3 ] );

			( ( float * )( entityData + resultingField->offset ) ) [ 0 ] = vec4[ 0 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 1 ] = vec4[ 1 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 2 ] = vec4[ 2 ];
			( ( float * )( entityData + resultingField->offset ) ) [ 3 ] = vec4[ 3 ];
			break;

		case F_INT:
			* ( int * )( entityData + resultingField->offset ) = atoi( value );
			break;

		case F_FLOAT:
			* ( float * )( entityData + resultingField->offset ) = atof( value );
			break;

		case F_YAW:
			v = atof( value );
			( ( float * )( entityData + resultingField->offset ) ) [ 0 ] = 0;
			( ( float * )( entityData + resultingField->offset ) ) [ 1 ] = v;
			( ( float * )( entityData + resultingField->offset ) ) [ 2 ] = 0;
			break;
	}

	if ( resultingField->replacement && resultingField->versionState )
		G_WarnAboutDeprecatedEntityField(entity, resultingField->replacement, key, resultingField->versionState );
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void )
{
	int       i, j;
	gentity_t *ent;

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0; i < level.numSpawnVars; i++ )
	{
		G_ParseField( level.spawnVars[ i ][ 0 ], level.spawnVars[ i ][ 1 ], ent );
	}

	G_SpawnInt( "notq3a", "0", &i );

	if ( i )
	{
		G_FreeEntity( ent );
		return;
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// don't leave any "gaps" between multiple targets
	j = 0;
	for (i = 0; i < MAX_TARGETS; ++i)
	{
		if (ent->targets[i])
			ent->targets[j++] = ent->targets[i];
	}
	ent->targets[ j ] = NULL;

	// don't leave any "gaps" between multiple targetnames
	j = 0;
	for (i = 0; i < MAX_TARGETNAMES; ++i)
	{
		if (ent->targetnames[i])
			ent->targetnames[j++] = ent->targetnames[i];
	}
	ent->targetnames[ j ] = NULL;

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) )
	{
		G_FreeEntity( ent );
	}
}

qboolean G_WarnAboutDeprecatedEntityField( gentity_t *entity, const char *expectedFieldname, const char *actualFieldname, const int typeOfDeprecation  )
{
	if ( !Q_stricmp(expectedFieldname, actualFieldname) || typeOfDeprecation == ENT_V_CURRENT )
		return qfalse;

	if ( g_debugEntities.integer >= 0 ) //dont't warn about anything with -1 or lower
	{
		if( typeOfDeprecation < ENT_V_TMPORARY
		|| ( g_debugEntities.integer >= 1 && typeOfDeprecation >= ENT_V_TMPORARY) )
		{
			G_Printf( "^3WARNING: ^7deprecated entity fieldname ^5%s^7 in ^5%s^7 found — use ^5%s^7 instead\n", actualFieldname, entity->classname, expectedFieldname );
		}
	}

	return qtrue;
}

/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string )
{
	int  l;
	char *dest;

	l = strlen( string );

	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS )
	{
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void )
{
	char keyname[ MAX_TOKEN_CHARS ];
	char com_token[ MAX_TOKEN_CHARS ];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) )
	{
		// end of spawn string
		return qfalse;
	}

	if ( com_token[ 0 ] != '{' )
	{
		G_Error( "G_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while ( 1 )
	{
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) )
		{
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[ 0 ] == '}' )
		{
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) )
		{
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[ 0 ] == '}' )
		{
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}

		if ( level.numSpawnVars == MAX_SPAWN_VARS )
		{
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}

		level.spawnVars[ level.numSpawnVars ][ 0 ] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][ 1 ] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED worldspawn (0 0 0) ?
Every map should have exactly one worldspawn, which is used for global options.

message: Text to print during connection process. Used for the name of level.
music: path/name of looping .wav file used for level's music (eg. music/sonic5.wav).
gravity: level gravity [g_gravity (800)]

humanBuildPoints: maximum amount of power the humans can use. [g_humanBuildPoints]
humanRepeaterBuildPoints: maximum amount of power the humans can use around each repeater. [g_humanRepeaterBuildPoints]
alienBuildPoints: maximum amount of sentience available to the overmind. [g_alienBuildPoints]

humanMaxStage: The highest stage the humans are allowed to use (0/1/2). [g_alienMaxStage (2)]
alienMaxStage: The highest stage the aliens are allowed to use (0/1/2). [g_humanMaxStage (2)]

disabledEquipment: A comma delimited list of human weapons or upgrades to disable for this map. [g_disabledEquipment ()]
disabledClasses: A comma delimited list of alien classes to disable for this map. [g_disabledClasses ()]
disabledBuildables: A comma delimited list of buildables to disable for this map. [g_disabledBuildables ()]
*/
void SP_worldspawn( void )
{
	char *s;

	G_SpawnString( "classname", "", &s );

	if ( Q_stricmp( s, "worldspawn" ) )
	{
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );  // map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );  // message of the day

	if ( G_SpawnString( "gravity", "", &s ) )
	{
		trap_Cvar_Set( "g_gravity", s );
	}

	if ( G_SpawnString( "humanMaxStage", "", &s ) )
	{
		trap_Cvar_Set( "g_humanMaxStage", s );
	}

	if ( G_SpawnString( "alienMaxStage", "", &s ) )
	{
		trap_Cvar_Set( "g_alienMaxStage", s );
	}

	if ( G_SpawnString( "humanRepeaterBuildPoints", "", &s ) )
		trap_Cvar_Set( "g_humanRepeaterBuildPoints", s );

	if ( G_SpawnString( "humanBuildPoints", "", &s ) )
		trap_Cvar_Set( "g_humanBuildPoints", s );

	if ( G_SpawnString( "alienBuildPoints", "", &s ) )
		trap_Cvar_Set( "g_alienBuildPoints", s );

	G_SpawnString( "disabledEquipment", "", &s );
	trap_Cvar_Set( "g_disabledEquipment", s );

	G_SpawnString( "disabledClasses", "", &s );
	trap_Cvar_Set( "g_disabledClasses", s );

	G_SpawnString( "disabledBuildables", "", &s );
	trap_Cvar_Set( "g_disabledBuildables", s );

	g_entities[ ENTITYNUM_WORLD ].s.number = ENTITYNUM_WORLD;
	g_entities[ ENTITYNUM_WORLD ].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_WORLD ].classname = "worldspawn";

	g_entities[ ENTITYNUM_NONE ].s.number = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_NONE ].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ ENTITYNUM_NONE ].classname = "nothing";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "-1" );

	if ( g_doWarmup.integer )
	{
		level.warmupTime = level.time - level.startTime + ( g_warmup.integer * 1000 );
		trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		G_LogPrintf( "Warmup: %i\n", g_warmup.integer );
	}

	level.timelimit = g_timelimit.integer;
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void )
{
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() )
	{
		G_Error( "SpawnEntities: no entities" );
	}

	SP_target_init();
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() )
	{
		G_SpawnGEntityFromSpawnVars();
	}
}
