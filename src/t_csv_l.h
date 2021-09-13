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
	  T_CSV_FLDBEG     = 0     ///< begin new field
	, T_CSV_DATQTE     = 1     ///< data inside quotes
	, T_CSV_DATNKD     = 2     ///< data without quotes
	, T_CSV_EMPTY      = 3     ///< end actual value
	, T_CSV_QTE1ST     = 4     ///< first at field begin
	, T_CSV_QTE2ND     = 5     ///< end of field
	, T_CSV_QTEONE     = 6     ///< end actual value
	, T_CSV_QTETWO     = 7     ///< first quote inseq
	, T_CSV_RECDNE     = 8     ///< Redord Finished
};

const char* t_csv_ste_nme[ ] = {
	  "FieldBegin"
	, "DataQuoted"
	, "DataNaked"
	, "EmptyData"
	, "Quotes1st"
	, "Quotes2nd"
	, "QuotesOne"
	, "QuotesTwo"
	, "RecordDone"
	, NULL
};


#define T_CSV_HDRIDX    1
/// The userdata struct for T.Csv row parser
struct t_csv
{
	char              dlm;    ///< Tsv/Csv delimiter character string, NUL terminated
	char              qte;    ///< quotation string, NUL terminated
	char              esc;    ///< Tsv/Csv escape character string, NUL terminated
	int               dbl;    ///< Use double quotation
	enum t_csv_ste    ste;    ///< Current parse state
	const char       *fld;    ///< Current start of field
};

// Constructors
int             luaopen_t_csv  ( lua_State *L );
struct t_csv   *t_csv_check_ud ( lua_State *L, int pos, int check );
struct t_csv   *t_csv_create_ud( lua_State *L, FILE fil, char sep, char esc );
