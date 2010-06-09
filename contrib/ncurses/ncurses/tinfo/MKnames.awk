##############################################################################
# Copyright (c) 2007,2008 Free Software Foundation, Inc.                     #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
# $Id$
function large_item(value) {
	result = sprintf("%d,", offset);
	offset = offset + length(value) + 1;
	offcol = offcol + length(result) + 2;
	if (offcol > 70) {
		result = result "\n";
		offcol = 0;
	} else {
		result = result " ";
	}
	bigstr = bigstr sprintf("\"%s\\0\" ", value);
	bigcol = bigcol + length(value) + 5;
	if (bigcol > 70) {
		bigstr = bigstr "\\\n";
		bigcol = 0;
	}
	return result;
}

function small_item(value) {
	return sprintf("\t\t\"%s\",\n", value);
}

function print_strings(name,value) {
	printf  "DCL(%s) = {\n", name
	print  value
	print  "\t\t(NCURSES_CONST char *)0,"
	print  "};"
	print  ""
}

function print_offsets(name,value) {
	printf  "static const short _nc_offset_%s[] = {\n", name
	printf "%s",  value
	print  "};"
	print  ""
	printf "static NCURSES_CONST char ** ptr_%s = 0;\n", name
	print  ""
}

BEGIN	{
		print  "/* This file was generated by MKnames.awk */"
		print  ""
		print  "#include <curses.priv.h>"
		print  ""
		print  "#define IT NCURSES_CONST char * const"
		print  ""
		offset = 0;
		offcol = 0;
		bigcol = 0;
	}

$1 ~ /^#/		{next;}

$1 == "SKIPWARN"	{next;}

$3 == "bool"	{
			small_boolnames = small_boolnames small_item($2);
			large_boolnames = large_boolnames large_item($2);
			small_boolfnames = small_boolfnames small_item($1);
			large_boolfnames = large_boolfnames large_item($1);
		}

$3 == "num"	{
			small_numnames = small_numnames small_item($2);
			large_numnames = large_numnames large_item($2);
			small_numfnames = small_numfnames small_item($1);
			large_numfnames = large_numfnames large_item($1);
		}

$3 == "str"	{
			small_strnames = small_strnames small_item($2);
			large_strnames = large_strnames large_item($2);
			small_strfnames = small_strfnames small_item($1);
			large_strfnames = large_strfnames large_item($1);
		}

END	{
		print  ""
		print  "#if BROKEN_LINKER || USE_REENTRANT"
		print  ""
		print  "#include <term.h>"
		print  ""
		if (bigstrings) {
			printf "static const char _nc_name_blob[] = \n"
			printf "%s;\n", bigstr;
			print_offsets("boolfnames", large_boolfnames);
			print_offsets("boolnames", large_boolnames);
			print_offsets("numfnames", large_numfnames);
			print_offsets("numnames", large_numnames);
			print_offsets("strfnames", large_strfnames);
			print_offsets("strnames", large_strnames);
			print  ""
			print  "static IT *"
			print  "alloc_array(NCURSES_CONST char ***value, const short *offsets, unsigned size)"
			print  "{"
			print  "	if (*value == 0) {"
			print  "		if ((*value = typeCalloc(NCURSES_CONST char *, size + 1)) != 0) {"
			print  "			unsigned n;"
			print  "			for (n = 0; n < size; ++n) {"
			print  "				(*value)[n] = (NCURSES_CONST char *) _nc_name_blob + offsets[n];"
			print  "			}"
			print  "		}"
			print  "	}"
			print  "	return *value;"
			print  "}"
			print  ""
			print  "#define FIX(it) NCURSES_IMPEXP IT * NCURSES_API _nc_##it(void) { return alloc_array(&ptr_##it, _nc_offset_##it, SIZEOF(_nc_offset_##it)); }"
		} else {
			print  "#define DCL(it) static IT data##it[]"
			print  ""
			print_strings("boolnames", small_boolnames);
			print_strings("boolfnames", small_boolfnames);
			print_strings("numnames", small_numnames);
			print_strings("numfnames", small_numfnames);
			print_strings("strnames", small_strnames);
			print_strings("strfnames", small_strfnames);
			print  "#define FIX(it) NCURSES_IMPEXP IT * NCURSES_API _nc_##it(void) { return data##it; }"
		}
		print  ""
		print  "FIX(boolnames)"
		print  "FIX(boolfnames)"
		print  "FIX(numnames)"
		print  "FIX(numfnames)"
		print  "FIX(strnames)"
		print  "FIX(strfnames)"
		print  ""
		print  ""
		print  "#define FREE_FIX(it) if (ptr_##it) { FreeAndNull(ptr_##it); }"
		print  ""
		print  "#if NO_LEAKS"
		print  "NCURSES_EXPORT(void)"
		print  "_nc_names_leaks(void)"
		print  "{"
		if (bigstrings) {
		print  "FREE_FIX(boolnames)"
		print  "FREE_FIX(boolfnames)"
		print  "FREE_FIX(numnames)"
		print  "FREE_FIX(numfnames)"
		print  "FREE_FIX(strnames)"
		print  "FREE_FIX(strfnames)"
		}
		print  "}"
		print  "#endif"
		print  ""
		print  "#else"
		print  ""
		print  "#define DCL(it) NCURSES_EXPORT_VAR(IT) it[]"
		print  ""
		print_strings("boolnames", small_boolnames);
		print_strings("boolfnames", small_boolfnames);
		print_strings("numnames", small_numnames);
		print_strings("numfnames", small_numfnames);
		print_strings("strnames", small_strnames);
		print_strings("strfnames", small_strfnames);
		print  ""
		print  "#endif /* BROKEN_LINKER */"
	}
