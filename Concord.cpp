 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Université Paris-Est Marne-la-Vallée <unitex@univ-mlv.fr>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Unicode.h"
#include "Text_tokens.h"
#include "String_hash.h"
#include "List_int.h"
#include "Alphabet.h"
#include "Matches.h"
#include "Concordance.h"
#include "File.h"
#include "Copyright.h"
#include "LocatePattern.h"
#include "Error.h"
#include "Snt.h"
#include "Copyright.h"
#include "getopt.h"
#include "ProgramInvoker.h"
#include "Concord.h"



static void usage_() {
u_printf("%S",COPYRIGHT);
u_printf("Usage: Concord [OPTIONS] <concord>\n"
        "\n"
        "  <concord>: a concord.ind file generated by the locate program\n"
        "\n"
        "OPTIONS:\n"
        "  -f FONT/--font=FONT: the name of the font to use if the output is an HTML file\n"
        "  -s N/--fontsize=N: the font size to use if the output is an HTML file\n"
        "  -l X/--left=X: left context length in chars; if followed by s (i.e. \"80s\") indicates\n"
        "                 that the match can end before the limit if there is a {S}. Default=0\n"
        "  -r X/--right=X: the same for right context. Default=0\n"
        "\n"
        "Sort order options:\n"
        "  --TO: text order (default)\n"
        "  --LC: left, center\n"
        "  --LR: left, right\n"
        "  --CL: center, left\n"
        "  --CR: center, right\n"
        "  --RL: right, left\n"
        "  --RC: right, center\n"
        "\n"
        "Output options:\n"
        "  -H/--html: produces an HTML concordance file (default)\n"
        "  -t/--text: produces a plain text concordance file\n"
        "  -g SCRIPT/--glossanet=SCRIPT: produces a glossanet HTML concordance file\n"
		"  -p SCRIPT/--script=SCRIPT: produces a HTML concordance file where occurrences\n"
		"                             are links described by SCRIPT\n"
        "  -i/--index: produces an index of the concordance\n"
        "  -u/--uima: produces another index of the concordance\n"
        "  -A/--axis: produces an axis file for the concordance (cf. [Melamed 06])\n"
        "  -x/--xalign: produces an index file for XAlign display\n"
        "  -m TXT/--merge=TXT: produces a file named TXT which is the SNT file\n"
        "                      merged with the match results\n"
        "\n"
        "  -d DIR/--directory=DIR: does not work in the same directory than <concord> but in DIR\n"
        "  -a ALPH/--alphabet=ALPH : the char order file used for sorting\n"
        "  -T/--thai: option to use for Thai concordances\n"
        "  -h/--help: this help\n"
        "\n"
        "Extracts the matches stored in <concord>, and stores them into a UTF-8\n"
        "HTML file saved in the <concord> directory or produces a text file, according\n"
        "to the mode parameter\n"
        "\nExamples:\n"
        "Concord tutu.ind \"-fCourier New\" -s12 -l40 -r40 --TO --html \"-ad:\\My dir\\alph.txt\"\n"
        "    -> produces an HTML file\n"
        "\n"
        "Concord tutu.ind \"--merge=C:\\My dir\\RES.SNT\"\n"
        "    -> produces a text file named \"C:\\My dir\\RES.SNT\"\n");
}



int pseudo_main_Concord(const char* index_file,const char* font,int fontsize,
                        int left_context,int right_context,const char* sort_order,
                        const char* output,const char* directory,const char* alphabet,int thai) {
ProgramInvoker* invoker=new_ProgramInvoker(main_Concord,"main_Concord");
char tmp[256];
if (font!=NULL) {
   add_argument(invoker,"-f");
   add_argument(invoker,font);
   sprintf(tmp,"%d",fontsize);
   add_argument(invoker,"-s");
   add_argument(invoker,tmp);
}
sprintf(tmp,"%d",left_context);
add_argument(invoker,"-l");
add_argument(invoker,tmp);
sprintf(tmp,"%d",right_context);
add_argument(invoker,"-r");
add_argument(invoker,tmp);
if (sort_order==NULL) {
   add_argument(invoker,"--TO");
} else {
   add_argument(invoker,sort_order);
}
add_argument(invoker,output);
if (directory!=NULL) {
   add_argument(invoker,"-d");
   add_argument(invoker,directory);
}
if (alphabet!=NULL) {
   add_argument(invoker,"-a");
   add_argument(invoker,alphabet);
}
if (thai) {
   add_argument(invoker,"-T");
}
add_argument(invoker,index_file);
int ret=invoke(invoker);
free_ProgramInvoker(invoker);
return ret;
}



/**
 * The same than main, but no call to setBufferMode.
 */
int main_Concord(int argc,char* argv[]) {
if (argc==1) {
   usage_();
   return 0;
}

const char* optstring=":f:s:l:r:Htg:p:iuAxm:a:Td:h";
const struct option_TS lopts[]= {
      {"font",required_argument_TS,NULL,'f'},
      {"fontsize",required_argument_TS,NULL,'s'},
      {"left",required_argument_TS,NULL,'l'},
      {"right",required_argument_TS,NULL,'r'},
      {"TO",no_argument_TS,NULL,0},
      {"LC",no_argument_TS,NULL,1},
      {"LR",no_argument_TS,NULL,2},
      {"CL",no_argument_TS,NULL,3},
      {"CR",no_argument_TS,NULL,4},
      {"RL",no_argument_TS,NULL,5},
      {"RC",no_argument_TS,NULL,6},
      {"html",no_argument_TS,NULL,'H'},
      {"text",no_argument_TS,NULL,'t'},
      {"glossanet",required_argument_TS,NULL,'g'},
      {"script",required_argument_TS,NULL,'p'},
      {"index",no_argument_TS,NULL,'i'},
      {"uima",no_argument_TS,NULL,'u'},
      {"axis",no_argument_TS,NULL,'A'},
      {"xalign",no_argument_TS,NULL,'x'},
      {"merge",required_argument_TS,NULL,'m'},
      {"alphabet",required_argument_TS,NULL,'a'},
      {"thai",no_argument_TS,NULL,'T'},
      {"directory",required_argument_TS,NULL,'d'},
      {"help",no_argument_TS,NULL,'h'},
      {NULL,no_argument_TS,NULL,0}
};
int val,index=-1;
struct conc_opt* options=new_conc_opt();
char foo;
int ret;
struct OptVars* vars=new_OptVars();
while (EOF!=(val=getopt_long_TS(argc,argv,optstring,lopts,&index,vars))) {
   switch(val) {
   case 'f': if (vars->optarg[0]=='\0') {
                fatal_error("Empty font name argument\n");
             }
             options->fontname=strdup(vars->optarg);
             if (options->fontname==NULL) {
                fatal_alloc_error("main_Concord");
             }
             break;
   case 's': if (1!=sscanf(vars->optarg,"%d%c",&(options->fontsize),&foo)) {
                /* foo is used to check that the font size is not like "45gjh" */
                fatal_error("Invalid font size argument: %s\n",vars->optarg);
             }
             break;
   case 'l': ret=sscanf(vars->optarg,"%d%c%c",&(options->left_context),&foo,&foo);
             if (ret==0 || ret==3 || (ret==2 && foo!='s') || options->left_context<0) {
                fatal_error("Invalid left context argument: %s\n",vars->optarg);
             }
             if (ret==2) {
                options->left_context_until_eos=1;
             }
             break;
   case 'r': ret=sscanf(vars->optarg,"%d%c%c",&(options->right_context),&foo,&foo);
             if (ret==0 || ret==3 || (ret==2 && foo!='s') || options->right_context<0) {
                fatal_error("Invalid right context argument: %s\n",vars->optarg);
             }
             if (ret==2) {
                options->right_context_until_eos=1;
             }
             break;
   case 0: options->sort_mode=TEXT_ORDER; break;
   case 1: options->sort_mode=LEFT_CENTER; break;
   case 2: options->sort_mode=LEFT_RIGHT; break;
   case 3: options->sort_mode=CENTER_LEFT; break;
   case 4: options->sort_mode=CENTER_RIGHT; break;
   case 5: options->sort_mode=RIGHT_LEFT; break;
   case 6: options->sort_mode=RIGHT_CENTER; break;
   case 'H': options->result_mode=HTML_; break;
   case 't': options->result_mode=TEXT_; break;
   case 'g': options->result_mode=GLOSSANET_;
             if (vars->optarg[0]=='\0') {
                fatal_error("Empty glossanet script argument\n");
             }
             options->script=strdup(vars->optarg);
             if (options->script==NULL) {
                fatal_alloc_error("main_Concord");
             }
             break;
   case 'p': options->result_mode=SCRIPT_;
             if (vars->optarg[0]=='\0') {
                fatal_error("Empty script argument\n");
             }
             options->script=strdup(vars->optarg);
             if (options->script==NULL) {
                fatal_alloc_error("main_Concord");
             }
             break;
   case 'i': options->result_mode=INDEX_; break;
   case 'u': options->result_mode=UIMA_; break;
   case 'A': options->result_mode=AXIS_; break;
   case 'x': options->result_mode=XALIGN_; break;
   case 'm': options->result_mode=MERGE_;
             if (vars->optarg[0]=='\0') {
                fatal_error("Empty output file name argument\n");
             }
             strcpy(options->output,vars->optarg);
             break;
   case 'a': if (vars->optarg[0]=='\0') {
                fatal_error("Empty alphabet argument\n");
             }
             options->sort_alphabet=strdup(vars->optarg);
             if (options->sort_alphabet==NULL) {
                fatal_alloc_error("main_Concord");
             }
             break;
   case 'T': options->thai_mode=1; break;
   case 'd': if (vars->optarg[0]=='\0') {
                fatal_error("Empty snt directory argument\n");
             }
             strcpy(options->working_directory,vars->optarg);
             break;
   case 'h': usage_(); return 0;
   case ':': if (index==-1) fatal_error("Missing argument for option -%c\n",vars->optopt);
             else fatal_error("Missing argument for option --%s\n",lopts[index].name);
   case '?': if (index==-1) fatal_error("Invalid option -%c\n",vars->optopt);
             else fatal_error("Invalid option --%s\n",vars->optarg);
             break;
   }
   index=-1;
}
if (vars->optind!=argc-1) {
   error("Invalid arguments: rerun with --help\n");
   return 1;
}

if (options->fontname==NULL || options->fontsize<=0) {
   if (options->result_mode==HTML_ || options->result_mode==GLOSSANET_) {
      fatal_error("The specified output mode is an HTML file: you must specify font parameters\n");
   }
}
U_FILE* concor=u_fopen(UTF16_LE,argv[vars->optind],U_READ);
if (concor==NULL) {
   error("Cannot open concordance index file %s\n",argv[vars->optind]);
   return 1;
}

if (options->working_directory[0]=='\0') {
   get_path(argv[vars->optind],options->working_directory);
}
/* We compute the name of the files associated to the text */
struct snt_files* snt_files=new_snt_files_from_path(options->working_directory);
U_FILE* text=u_fopen(BINARY,snt_files->text_cod,U_READ);
if (text==NULL) {
	error("Cannot open file %s\n",snt_files->text_cod);
	u_fclose(concor);
	free_snt_files(snt_files);
	return 1;
}
struct text_tokens* tok=load_text_tokens(snt_files->tokens_txt);
if (tok==NULL) {
	error("Cannot load text token file %s\n",snt_files->tokens_txt);
	u_fclose(concor);
	u_fclose(text);
	free_snt_files(snt_files);
	return 1;
}

U_FILE* f_enter=u_fopen(BINARY,snt_files->enter_pos,U_READ);
int n_enter_char;
/* New lines are encoded in 'enter.pos' files. Those files will disappear in the future */
int* enter_pos;
long size=get_file_size(f_enter);
enter_pos=(int*)malloc(size);
if (enter_pos==NULL) {
	fatal_alloc_error("main_Concord");
}
if (f_enter==NULL) {
	error("Cannot open file %s\n",snt_files->enter_pos);
	n_enter_char=0;
}
else {
	n_enter_char=(int)fread(enter_pos,sizeof(int),size/sizeof(int),f_enter);
	if (n_enter_char!=(int)(size/sizeof(int))) {
		fatal_error("Read error on enter.pos file in main_Concord\n");
	}
   u_fclose(f_enter);
}
if (options->result_mode==INDEX_ || options->result_mode==UIMA_ || options->result_mode==AXIS_) {
   /* We force some options for index, uima and axis files */
   options->left_context=0;
   options->right_context=0;
   options->sort_mode=TEXT_ORDER;
}

/* Once we have setted all the parameters, we call the function that
 * will actually create the concordance. */
create_concordance(concor,text,tok,n_enter_char,enter_pos,options);
free(enter_pos);
u_fclose(concor);
u_fclose(text);
free_snt_files(snt_files);
free_text_tokens(tok);
free_conc_opt(options);
free_OptVars(vars);
u_printf("Done.\n");
return 0;
}

