---
-- \file    test/t_csv_extern.lua
-- \brief   CSV Benchmark from npm package csv-parser
local Test     = require't.Test'
local Csv      = require't.Csv'
local t_insert = table.insert

local files = {
  { todo=false  , name = 'backtick.csv'                  , lines = 2    },
  { todo=false  , name = 'bad-data.csv'                  , lines = 3    },
  { todo=false  , name = 'basic.csv'                     , lines = 1    },
  { todo=false  , name = 'comma-in-quote.csv'            , lines = 1    },
  { todo=false  , name = 'comment.csv'                   , lines = 2    },
  { todo=false  , name = 'empty-columns.csv'             , lines = 1    },
  { todo=false  , name = 'escape-quotes.csv'             , lines = 3    },
  { todo=false  , name = 'geojson.csv'                   , lines = 3    },
  { todo=false  , name = 'headers.csv'                   , lines = 2    },
  { todo=false  , name = 'large-dataset.csv'             , lines = 7268 },
  { todo=false  , name = 'newlines.csv'                  , lines = 3    },
  { todo=false  , name = 'no-headers.csv'                , lines = 3    },
  { todo=false  , name = 'option-comment.csv'            , lines = 2    },
  { todo=false  , name = 'option-escape.csv'             , lines = 3    },
  { todo=false  , name = 'option-newline.csv'            , lines = 0    },
  { todo=false  , name = 'option-quote-escape.csv'       , lines = 3    },
  { todo=false  , name = 'option-quote-many.csv'         , lines = 3    },
  { todo=false  , name = 'option-quote.csv'              , lines = 2    },
  { todo=false  , name = 'quotes+newlines.csv'           , lines = 3    },
  { todo=false  , name = 'strict+skipLines.csv'          , lines = 4    },
  { todo=false  , name = 'strict-false-less-columns.csv' , lines = 3    },
  { todo=false  , name = 'strict-false-more-columns.csv' , lines = 3    },
  { todo=false  , name = 'strict.csv'                    , lines = 3    },
  { todo=false  , name = 'latin.csv'                     , lines = 2    },
  { todo=true   , name = 'mac-newlines.csv'              , lines = 2    },
  { todo=true   , name = 'utf16-big.csv'                 , lines = 2    },
  { todo=true   , name = 'utf16.csv'                     , lines = 2    },
  { todo=false  , name = 'utf8.csv'                      , lines = 2    },
}

local tests = {}
for _,file in ipairs( files ) do
	local fn = function( self )
		Test.describe( "Parsing file %s with expected %d lines", file.name, file.lines )
		if file.todo then
			Test.todo( "Not implemented" )
		end
		local csv,data,cnt     = Csv( {separator=",", headers=true, skip=0} ), {}, 0
		for row, i in csv:rows( io.lines( 'test/csv/' .. file.name ) ) do
			t_insert( data, row )
			cnt = i
		end
		assert( cnt == file.lines, ("Expected %d rows but parsed %d "):format( file.lines, cnt ) )
	end
	t_insert( tests, fn )
end

return tests

