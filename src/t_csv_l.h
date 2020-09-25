// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/**
 * \file      t_csv.h
 * \brief     Reading and writing CSV/TSV files
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t_csv.h"
#include "t.h"             // UNUSED()

/// The userdata struct for T.Csv
struct t_csv
{
	FILE           fil;     ///< FileHandle
	char           sep;     ///< Tsv/Csv separator character
	char           esc;     ///< Tsv/Csv Quote Escape character
};
#define ISDIGIT( c ) ((c) - '0' + 0U <= 9U)

enum t_csv_ste {
	  T_CSV_FLDSTART      ///< 0 start a brand new field
	, T_CSV_FLDEND        ///< 1 reached end of a field
	, T_CSV_NOQOUTE       ///< 2 start of a non quoted field
	, T_CSV_INQUOTES      ///< 3 within a quoted field
	, T_CSV_ROWEND        ///< 4 reached end of a data row
	, T_CSV_ROWTRUNCED    ///< 5 must read more data to continue parsing field
	, T_CSV_ROWDONE       ///< 6 return control
};
char* states[ ] = { "READY","NOQTE","INQTE","F_END","R_END","MORED","R_DNE" };


// Constructors
int             luaopen_t_csv  ( lua_State *L );
struct t_csv   *t_csv_check_ud ( lua_State *L, int pos, int check );
struct t_csv   *t_csv_create_ud( lua_State *L, FILE fil, char sep, char esc );
