// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/**
 * \file      t_csv.h
 * \brief     Reading and writing CSV/TSV files
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_csv.h"
#include "t.h"             // t_typeerror()

enum t_csv_ste {
	  T_CSV_FLDSTART   = 0  ///< 0 start a brand new field
	, T_CSV_FLDEND     = 1  ///< 1 reached end of a field
	, T_CSV_NOQOUTE    = 2  ///< 2 start of a non quoted field
	, T_CSV_INQUOTES   = 3  ///< 3 within a quoted field
	, T_CSV_ROWTRUNCED = 4  ///< 4 must read more data to continue parsing field
	, T_CSV_ROWDONE    = 5  ///< 5 return control
};
const char* t_csv_ste_nme[ ] = {
	  "FieldStart"
	, "FieldEnd"
	, "NoQuotes"
	, "InQoutes"
	, "RowTruncated"
	, "RowDone"
	, NULL
};

/// The userdata struct for T.Csv row parser
struct t_csv
{
	enum t_csv_ste    ste;  ///< Current parse state
	char           dlm[2];  ///< Tsv/Csv delimiter character string, NUL terminated
	char           qts[2];  ///< quotation string, NUL terminated
	char           qtd[3];  ///< quotation string for substitution (doublequotes), NUL terminated
	char           esc[2];  ///< Tsv/Csv escape character string, NUL terminated
	int               dbl;  ///< Use double quotation
	lua_Integer        hR;  ///< Reference to file handle in LUA_REGISTRYINDEX
	lua_Integer       cnt;  ///< running field count of current line
	size_t            len;  ///< Length of current line load
	const char       *lne;  ///< Current line load
	char             *fld;  ///< Current start of field
	char             *run;  ///< Current runner position in lne
};


// Constructors
int             luaopen_t_csv  ( lua_State *L );
struct t_csv   *t_csv_check_ud ( lua_State *L, int pos, int check );
struct t_csv   *t_csv_create_ud( lua_State *L, FILE fil, char sep, char esc );
