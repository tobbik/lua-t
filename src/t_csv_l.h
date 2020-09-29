// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/**
 * \file      t_csv.h
 * \brief     Reading and writing CSV/TSV files
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t_csv.h"
#include "t.h"             // UNUSED()


enum t_csv_ste {
	  T_CSV_FLDSTART      ///< 0 start a brand new field
	, T_CSV_FLDEND        ///< 1 reached end of a field
	, T_CSV_NOQOUTE       ///< 2 start of a non quoted field
	, T_CSV_INQUOTES      ///< 3 within a quoted field
	, T_CSV_ROWTRUNCED    ///< 5 must read more data to continue parsing field
	, T_CSV_ROWDONE       ///< 6 return control
};
char* states[ ] = { "F_SRT", "F_END", "NOQTE","INQTE","R_TRC","R_DNE" };

/// The userdata struct for T.Csv
struct t_csv
{
	enum t_csv_ste ste;     ///< Current parse state
	char           dlm;     ///< Tsv/Csv delimiter character
	char           qot;     ///< Quotation character
	char           esc;     ///< Tsv/Csv escape character
	int            dbl;     ///< Use double quotation
	char       dqot[3];     ///< quotation string for substitution
	char       sqot[2];     ///< quotation string for substitution
	int            cnt;     ///< running field count of current line
	size_t         len;     ///< Length of current line load
	const char    *lne;     ///< Current line load
	char          *fld;     ///< Current start of field
	char          *run;     ///< Current runner position in lne
};


// Constructors
int             luaopen_t_csv  ( lua_State *L );
struct t_csv   *t_csv_check_ud ( lua_State *L, int pos, int check );
struct t_csv   *t_csv_create_ud( lua_State *L, FILE fil, char sep, char esc );
