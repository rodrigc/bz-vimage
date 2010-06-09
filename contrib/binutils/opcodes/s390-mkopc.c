/* s390-mkopc.c -- Generates opcode table out of s390-opc.txt
   Copyright 2000, 2001 Free Software Foundation, Inc.
   Contributed by Martin Schwidefsky (schwidefsky@de.ibm.com).

   This file is part of GDB, GAS, and the GNU binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Taken from opcodes/s390.h */
enum s390_opcode_mode_val
  {
    S390_OPCODE_ESA = 0,
    S390_OPCODE_ZARCH
  };

enum s390_opcode_cpu_val
  {
    S390_OPCODE_G5 = 0,
    S390_OPCODE_G6,
    S390_OPCODE_Z900,
    S390_OPCODE_Z990
  };

struct op_struct
  {
    char  opcode[16];
    char  mnemonic[16];
    char  format[16];
    int   mode_bits;
    int   min_cpu;
    
    unsigned long long sort_value;
    int   no_nibbles;
  };

struct op_struct *op_array;
int max_ops;
int no_ops;

static void
createTable (void)
{
  max_ops = 256;
  op_array = malloc (max_ops * sizeof (struct op_struct));
  no_ops = 0;
}

/* `insertOpcode': insert an op_struct into sorted opcode array.  */

static void
insertOpcode (char *opcode, char *mnemonic, char *format,
	      int min_cpu, int mode_bits)
{
  char *str;
  unsigned long long sort_value;
  int no_nibbles;
  int ix, k;

  while (no_ops >= max_ops)
    {
      max_ops = max_ops * 2;
      op_array = realloc (op_array, max_ops * sizeof (struct op_struct));
    }

  sort_value = 0;
  str = opcode;
  for (ix = 0; ix < 16; ix++)
    {
      if (*str >= '0' && *str <= '9')
	sort_value = (sort_value << 4) + (*str - '0');
      else if (*str >= 'a' && *str <= 'f')
	sort_value = (sort_value << 4) + (*str - 'a' + 10);
      else if (*str >= 'A' && *str <= 'F')
	sort_value = (sort_value << 4) + (*str - 'A' + 10);
      else if (*str == '?')
	sort_value <<= 4;
      else
	break;
      str ++;
    }
  sort_value <<= 4*(16 - ix);
  sort_value += (min_cpu << 8) + mode_bits;
  no_nibbles = ix;
  for (ix = 0; ix < no_ops; ix++)
    if (sort_value > op_array[ix].sort_value)
      break;
  for (k = no_ops; k > ix; k--)
    op_array[k] = op_array[k-1];
  strcpy(op_array[ix].opcode, opcode);
  strcpy(op_array[ix].mnemonic, mnemonic);
  strcpy(op_array[ix].format, format);
  op_array[ix].sort_value = sort_value;
  op_array[ix].no_nibbles = no_nibbles;
  op_array[ix].min_cpu = min_cpu;
  op_array[ix].mode_bits = mode_bits;
  no_ops++;
}

static char file_header[] =
  "/* The opcode table. This file was generated by s390-mkopc.\n\n"
  "   The format of the opcode table is:\n\n"
  "   NAME	     OPCODE	MASK	OPERANDS\n\n"
  "   Name is the name of the instruction.\n"
  "   OPCODE is the instruction opcode.\n"
  "   MASK is the opcode mask; this is used to tell the disassembler\n"
  "     which bits in the actual opcode must match OPCODE.\n"
  "   OPERANDS is the list of operands.\n\n"
  "   The disassembler reads the table in order and prints the first\n"
  "   instruction which matches.  */\n\n"
  "const struct s390_opcode s390_opcodes[] =\n  {\n";

/* `dumpTable': write opcode table.  */

static void
dumpTable (void)
{
  char *str;
  int  ix;

  /*  Write hash table entries (slots).  */
  printf (file_header);

  for (ix = 0; ix < no_ops; ix++)
    {
      printf ("  { \"%s\", ", op_array[ix].mnemonic);
      for (str = op_array[ix].opcode; *str != 0; str++)
	if (*str == '?')
	  *str = '0';
      printf ("OP%i(0x%sLL), ", 
	      op_array[ix].no_nibbles*4, op_array[ix].opcode);
      printf ("MASK_%s, INSTR_%s, ",
	      op_array[ix].format, op_array[ix].format);
      printf ("%i, ", op_array[ix].mode_bits);
      printf ("%i}", op_array[ix].min_cpu);
      if (ix < no_ops-1)
	printf (",\n");
      else
	printf ("\n");
    }
  printf ("};\n\n");
  printf ("const int s390_num_opcodes =\n");
  printf ("  sizeof (s390_opcodes) / sizeof (s390_opcodes[0]);\n\n");
}

int
main (void)
{
  char currentLine[256];
  
  createTable ();

  /*  Read opcode descriptions from `stdin'.  For each mnemonic,
      make an entry into the opcode table.  */
  while (fgets (currentLine, sizeof (currentLine), stdin) != NULL)
    {
      char  opcode[16];
      char  mnemonic[16];
      char  format[16];
      char  description[64];
      char  cpu_string[16];
      char  modes_string[16];
      int   min_cpu;
      int   mode_bits;
      char  *str;

      if (currentLine[0] == '#')
        continue;
      memset (opcode, 0, 8);
      if (sscanf (currentLine, "%15s %15s %15s \"%[^\"]\" %15s %15s",
		  opcode, mnemonic, format, description,
		  cpu_string, modes_string) == 6)
	{
	  if (strcmp (cpu_string, "g5") == 0)
	    min_cpu = S390_OPCODE_G5;
	  else if (strcmp (cpu_string, "g6") == 0)
	    min_cpu = S390_OPCODE_G6;
	  else if (strcmp (cpu_string, "z900") == 0)
	    min_cpu = S390_OPCODE_Z900;
	  else if (strcmp (cpu_string, "z990") == 0)
	    min_cpu = S390_OPCODE_Z990;
	  else {
	    fprintf (stderr, "Couldn't parse cpu string %s\n", cpu_string);
	    exit (1);
	  }

	  str = modes_string;
	  mode_bits = 0;
	  do {
	    if (strncmp (str, "esa", 3) == 0
		&& (str[3] == 0 || str[3] == ',')) {
	      mode_bits |= 1 << S390_OPCODE_ESA;
	      str += 3;
	    } else if (strncmp (str, "zarch", 5) == 0
		       && (str[5] == 0 || str[5] == ',')) {
	      mode_bits |= 1 << S390_OPCODE_ZARCH;
	      str += 5;
	    } else {
	      fprintf (stderr, "Couldn't parse modes string %s\n",
		       modes_string);
	      exit (1);
	    }
	    if (*str == ',')
	      str++;
	  } while (*str != 0);
	  insertOpcode (opcode, mnemonic, format, min_cpu, mode_bits);
	}
      else
        fprintf (stderr, "Couldn't scan line %s\n", currentLine);
    }

  dumpTable ();
  return 0;
}
