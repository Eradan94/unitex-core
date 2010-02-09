 /*
  * Unitex
  *
  * Copyright (C) 2001-2010 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

/*
 * author : Anthony Sigogne
 */

#ifndef TrainingProcessH
#define TrainingProcessH

#include <stdio.h>
#include "Copyright.h"
#include "getopt.h"
#include "TaggingProcess.h"
#include "Error.h"
#include "File.h"
#include "DELA.h"
#include "Unicode.h"
#include "String_hash.h"
#include "SortTxt.h"
#include "Compress.h"

#define MAX_CONTEXT 3
#define SIMPLE_FORMS 0
#define COMPOUND_FORMS 1

struct corpus_entry{
	unichar* word;
	unichar* pos_code;
	unichar* overall_codes;
};

void free_corpus_entry(corpus_entry*);
void push_corpus_entry(corpus_entry*,corpus_entry**);
struct corpus_entry* create_corpus_entry(unichar*);
//void add_statistics(corpus_entry**,U_FILE*,U_FILE*);

void do_training(U_FILE*,U_FILE*,U_FILE*);

#endif
