 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
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

#include "Fst2.h"
#include "AbstractFst2Load.h"
#include "DELA.h"
#include "Pattern.h"
#include "LocateTfst_lib.h"
#include "File.h"
#include "Korean.h"

void explore_tfst(Tfst* tfst,int current_state_in_tfst,
		          int current_state_in_fst2,int depth,
		          struct tfst_match* match_element_list,
                  struct tfst_match_list** LIST,
                  struct locate_tfst_infos* infos,
                  int pos_kr_in_fst2_tag,
                  Transition* current_kr_fst2_transition);
void init_Korean_stuffs(struct locate_tfst_infos* infos,char* jamo_table);
void free_Korean_stuffs(struct locate_tfst_infos* infos);
void compute_jamo_tfst_tags(struct locate_tfst_infos* infos);
int match_between_text_and_grammar_tags(Tfst* tfst,TfstTag* text_tag,Fst2Tag grammar_tag,
                                        int tfst_tag_index,int fst2_tag_index,
                                        struct locate_tfst_infos* infos,int *pos_kr);
struct pattern* tokenize_grammar_tag(unichar* tag,int *negation);
int is_space_on_the_left_in_tfst(Tfst* tfst,TfstTag* tag);
int morphological_filter_is_ok(unichar* content,Fst2Tag grammar_tag,struct locate_tfst_infos* infos);


/**
 * This function applies the given grammar to the given text automaton.
 * It returns 1 in case of success; 0 otherwise.
 */
int locate_tfst(char* text,char* grammar,char* alphabet,char* output,MatchPolicy match_policy,
		        OutputPolicy output_policy,AmbiguousOutputPolicy ambiguous_output_policy,
		        int search_limit,char* jamo_table) {
Tfst* tfst=open_text_automaton(text);
if (tfst==NULL) {
	return 0;
}
struct FST2_free_info fst2_free;
struct locate_tfst_infos infos;
infos.fst2=load_abstract_fst2(grammar,0,&fst2_free);
if (infos.fst2==NULL) {
	close_text_automaton(tfst);
	return 0;
}
infos.tfst=tfst;
infos.number_of_matches=0;
int korean=(jamo_table!=NULL && jamo_table[0]!='\0');
infos.alphabet=load_alphabet(alphabet,korean);
if (infos.alphabet==NULL) {
	close_text_automaton(tfst);
	free_abstract_Fst2(infos.fst2,&fst2_free);
	error("Cannot load alphabet file: %s\n",alphabet);
	return 0;
}
infos.output=u_fopen(UTF16_LE,output,U_WRITE);
if (infos.output==NULL) {
	close_text_automaton(tfst);
	free_abstract_Fst2(infos.fst2,&fst2_free);
	free_alphabet(infos.alphabet);
	error("Cannot open %s\n",output);
	return 0;
}
infos.output_policy=output_policy;
switch (infos.output_policy) {
case IGNORE_OUTPUTS: u_fprintf(infos.output,"#I\n"); break;
case MERGE_OUTPUTS: u_fprintf(infos.output,"#M\n"); break;
case REPLACE_OUTPUTS: u_fprintf(infos.output,"#R\n"); break;
}
#ifdef TRE_WCHAR
infos.filters=new_FilterSet(infos.fst2,infos.alphabet);
infos.matches=NULL;
if (infos.filters==NULL) {
	close_text_automaton(tfst);
	free_abstract_Fst2(infos.fst2,&fst2_free);
	free_alphabet(infos.alphabet);
	u_fclose(infos.output);
    error("Cannot compile filter(s)\n");
   return 0;
}
#endif
infos.match_policy=match_policy;
infos.output_policy=output_policy;
infos.ambiguous_output_policy=ambiguous_output_policy;
infos.number_of_outputs=0;
infos.start_position_last_printed_match=-1;
infos.end_position_last_printed_match=-1;
infos.search_limit=search_limit;
init_Korean_stuffs(&infos,jamo_table);
/* We launch the matching for each sentence */
for (int i=1;i<=tfst->N && infos.number_of_matches!=infos.search_limit;i++) {
   if (i%100==0) {
		u_printf("\rSentence %d/%d...",i,tfst->N);
	}
	load_sentence(tfst,i);
	compute_token_contents(tfst);
	if (infos.korean) {
	   compute_jamo_tfst_tags(&infos);
	}
	infos.matches=NULL;
	/* Within a sentence graph, we try to match from any state */
	for (int j=0;j<tfst->automaton->number_of_states;j++) {
		explore_tfst(tfst,j,infos.fst2->initial_states[1],0,NULL,NULL,&infos,-1,NULL);
	}
	save_tfst_matches(&infos);
}
u_printf("\rDone.                                    \n");
/* We save some infos */
char concord_tfst_n[FILENAME_MAX];
get_path(output,concord_tfst_n);
strcat(concord_tfst_n,"concord_tfst.n");
U_FILE* f=u_fopen(UTF16_LE,concord_tfst_n,U_WRITE);
if (f==NULL) {
	error("Cannot save information in %s\n",concord_tfst_n);
} else {
	u_fprintf(f,"%d match%s",infos.number_of_matches,(infos.number_of_matches<=1)?"":"es");
	if ((infos.number_of_outputs != infos.number_of_matches)
	       && (infos.number_of_outputs != 0))
	     {
	       u_fprintf(f,"(%d output%s)\n",infos.number_of_outputs,(infos.number_of_outputs<=1)?"":"s");
	     }
	u_fprintf(f,"\n");
	u_fclose(f);
}
u_printf("%d match%s",infos.number_of_matches,(infos.number_of_matches<=1)?"":"es");
if ((infos.number_of_outputs != infos.number_of_matches)
       && (infos.number_of_outputs != 0))
     {
       u_printf("(%d output%s)\n",infos.number_of_outputs,(infos.number_of_outputs<=1)?"":"s");
     }
u_printf("\n");
/* And we free things */
#ifdef TRE_WCHAR
free_FilterSet(infos.filters);
#endif
u_fclose(infos.output);
free_alphabet(infos.alphabet);
free_Korean_stuffs(&infos);
free_abstract_Fst2(infos.fst2,&fst2_free);
close_text_automaton(tfst);
return 1;
}


/**
 * If we must deal with a Korean .tfst, we compute the jamo version of all fst2 tags.
 */
void init_Korean_stuffs(struct locate_tfst_infos* infos,char* jamo_table) {
if (jamo_table==NULL || jamo_table[0]=='\0') {
   infos->korean=0;
   infos->jamo=NULL;
   infos->n_jamo_fst2_tags=0;
   infos->jamo_fst2_tags=NULL;
   infos->n_jamo_tfst_tags=0;
   infos->jamo_tfst_tags=NULL;
   return;
}
infos->korean=1;
infos->jamo=new jamoCodage();
infos->n_jamo_tfst_tags=0;
infos->jamo_tfst_tags=NULL;
/* We initialize the Jamo table */
infos->jamo->loadJamoMap(jamo_table);
/* We also initializes the Chinese -> Hangul table */ 
infos->jamo->cloneHJAMap(infos->alphabet->korean_equivalent_syllab);
infos->n_jamo_fst2_tags=infos->fst2->number_of_tags;
infos->jamo_fst2_tags=(unichar**)malloc(sizeof(unichar*)*infos->n_jamo_fst2_tags);
if (infos->jamo_fst2_tags==NULL) {
   fatal_alloc_error("init_Korean_stuffs");
}
unichar tmp[4096];
for (int i=0;i<infos->n_jamo_fst2_tags;i++) {
   Fst2Tag t=infos->fst2->tags[i];
   if (t->input[0]=='{' && t->input[1]!='\0') {
      /* If we have a tag like {today,.ADV}, we compute the jamo version
       * of its inflected form */
      struct dela_entry* entry=tokenize_tag_token(t->input);
      if (entry==NULL) {
         fatal_error("NULL dela_entry in init_Korean_stuffs\n");
      }
      if (!u_strcmp(entry->inflected,"<E>")) {
         /* <E> is a special inflected form indicating that the surface form
          * is empty. In order to be careful, we prefer not to convert it */
         u_strcpy(tmp,"<E>");
      } else {
         convert_Korean_text(entry->inflected,tmp,infos->jamo,infos->alphabet);
      }
      free_dela_entry(entry);
   } else {
      if (i!=0) {
         convert_Korean_text(t->input,tmp,infos->jamo,infos->alphabet);
      } else {
         /* We don't want to convert the epsilon transition tag */
         u_strcpy(tmp,"<E>");
      }
   }
   infos->jamo_fst2_tags[i]=u_strdup(tmp);
   if (infos->jamo_fst2_tags[i]==NULL) {
      fatal_alloc_error("init_Korean_stuffs");
   }
}
}


/**
 * Frees memory associated to Korean stuffs.
 */
void free_Korean_stuffs(struct locate_tfst_infos* infos) {
if (!infos->korean) {
   return;
}
delete infos->jamo;
for (int i=0;i<infos->n_jamo_fst2_tags;i++) {
   free(infos->jamo_fst2_tags[i]);
}
free(infos->jamo_fst2_tags);
if (infos->jamo_tfst_tags!=NULL) {
   for (int i=0;i<infos->n_jamo_tfst_tags;i++) {
      free(infos->jamo_tfst_tags[i]);
   }
   free(infos->jamo_tfst_tags);
}
}


/**
 * Computes the Jamo versions of the current sentence automaton tags, freeing the previous
 * ones if any.
 */
void compute_jamo_tfst_tags(struct locate_tfst_infos* infos) {
if (infos->jamo_tfst_tags!=NULL) {
   for (int i=0;i<infos->n_jamo_tfst_tags;i++) {
      free(infos->jamo_tfst_tags[i]);
   }
   free(infos->jamo_tfst_tags);
}
infos->n_jamo_tfst_tags=infos->tfst->tags->nbelems;
infos->jamo_tfst_tags=(unichar**)malloc(sizeof(unichar*)*infos->n_jamo_tfst_tags);
if (infos->jamo_tfst_tags==NULL) {
   fatal_alloc_error("compute_jamo_tfst_tags");
}
unichar tmp[4096];
for (int i=0;i<infos->n_jamo_tfst_tags;i++) {
   TfstTag* t=(TfstTag*)(infos->tfst->tags->tab[i]);
   if (t->content[0]=='{' && t->content[1]!='\0') {
      /* If we have a tag like {today,.ADV}, we compute the jamo version
       * of its inflected form */
      struct dela_entry* entry=tokenize_tag_token(t->content);
      if (entry==NULL) {
         fatal_error("NULL dela_entry in compute_jamo_tfst_tags\n");
      }
      if (!u_strcmp(entry->inflected,"<E>")) {
         /* <E> is a special inflected form indicating that the surface form
          * is empty. In order to be careful, we prefer not to convert it */
         u_strcpy(tmp,"<E>");
      } else {
         convert_Korean_text(entry->inflected,tmp,infos->jamo,infos->alphabet);
      }
      free_dela_entry(entry);
   } else {
      if (!u_strcmp(t->content,"<E>")) {
         /* <E> is a special tag that should not be used by LocateTfst,
          * but, in order to be careful, we prefer not to convert it */
         u_strcpy(tmp,"<E>");
      } else {
         convert_Korean_text(t->content,tmp,infos->jamo,infos->alphabet);
      }
   }
   infos->jamo_tfst_tags[i]=u_strdup(tmp);
   if (infos->jamo_tfst_tags[i]==NULL) {
      fatal_alloc_error("compute_jamo_tfst_tags");
   }
}

}


/**
 * Explores in parallel the tfst and the fst2.
 */
void explore_tfst(Tfst* tfst,int current_state_in_tfst,
		          int current_state_in_fst2,int depth,
		          struct tfst_match* match_element_list,
                struct tfst_match_list* *LIST,
                struct locate_tfst_infos* infos,
                /* This is used for Korean only when a fst2 tag contains a token that
                 * can match several boxes in the tfst. 'pos_kr_in_fst2_tag' represents 
                 * the current position in the current fst2 tag, or -1 if unused. 
                 * 'current_kr_fst2_transition' is the current fst2 transition being explored if
                 * 'pos_kr_in_fst2_tag' is not -1; null otherwise. */
                int pos_kr_in_fst2_tag,
                Transition* current_kr_fst2_transition) {
if (current_kr_fst2_transition!=NULL) {
   /* If we have not finished to explore a fst2 tag in a Korean grammar */
   struct tfst_match* list=NULL;
   Transition* text_transition=tfst->automaton->states[current_state_in_tfst]->outgoing_transitions;
   /* For a given tag in the grammar, we test all the transitions in the
    * text automaton, and we note all the states we can reach */
   while (text_transition!=NULL) {
      int pos_kr=pos_kr_in_fst2_tag;
      int result=match_between_text_and_grammar_tags(tfst,(TfstTag*)(tfst->tags->tab[text_transition->tag_number]),
                                              infos->fst2->tags[current_kr_fst2_transition->tag_number],
                                              text_transition->tag_number,
                                              current_kr_fst2_transition->tag_number,
                                              infos,&pos_kr);
      if (result==OK_MATCH_STATUS) {
         /* Case of a match with something in the text automaton (i.e. <V>) */
         list=insert_in_tfst_matches(list,current_state_in_tfst,text_transition->state_number,
               current_kr_fst2_transition,pos_kr,text_transition->tag_number);
      }
      else if (result==TEXT_INDEPENDENT_MATCH) {
         /* Case of a match independent of the text automaton (i.e. <E>) */
         list=insert_in_tfst_matches(list,current_state_in_tfst,current_state_in_tfst,
               current_kr_fst2_transition,-1,NO_TEXT_TOKEN_WAS_MATCHED);
      }
      text_transition=text_transition->next;
   }
   struct tfst_match* tmp;
   /* Then, we continue the exploration from the reached states. This
    * procedure avoids exploring several times a same state when
    * it can be reached through various tags in the text automaton.
    * For instance, if we have "le" in the grammar and {le,.DET} and
    * {le,.PRO} in the text automaton that point to the same state XXX,
    * we will explore XXX just once. */
   while (list!=NULL) {
      tmp=list->next;
      list->next=match_element_list;
      /* match_element_list is pointed by one more element */
      if (match_element_list!=NULL) {
       (match_element_list->pointed_by)++;
      }
      Transition* tmp_trans=(list->pos_kr!=-1)?list->fst2_transition:NULL;
      int dest_state=(list->pos_kr!=-1)?-1:current_kr_fst2_transition->state_number;
      explore_tfst(tfst,list->dest_state_text,dest_state,
                        depth,list,LIST,infos,list->pos_kr,tmp_trans);
      if (list->pointed_by==0) {
         /* If list is not blocked by being part of a match for the calling
          * graph, we can free it */
         list->next=NULL;
         if (match_element_list!=NULL) {(match_element_list->pointed_by)--;}
         free_tfst_match(list);
      }
      list=tmp;
   }
   
   
   
   
   
   
   
   return;
}
/* We are in the normal state exploration case */
Fst2State current_state_in_grammar=infos->fst2->states[current_state_in_fst2];

if (is_final_state(current_state_in_grammar)) {
   /* If the current state is final */
   if (depth==0) {
      /* If we are in the main graph, we add a match to the main match list */
	   add_tfst_match(infos,match_element_list);
   } else {
      /* If we are in a subgraph, we add a match to the current match list */
      (*LIST)=add_match_in_list((*LIST),match_element_list);
   }
}

/* We test every transition that goes out the current state in the grammar */
Transition* grammar_transition=current_state_in_grammar->transitions;
while (grammar_transition!=NULL) {
   int e=grammar_transition->tag_number;
   if (e<0) {
      /* Case of a subgraph */
      struct tfst_match_list* list_for_subgraph=NULL;
      struct tfst_match_list* tmp;

      explore_tfst(tfst,current_state_in_tfst,infos->fst2->initial_states[-e],
              depth+1,match_element_list,&list_for_subgraph,infos,-1,NULL);
      while (list_for_subgraph!=NULL) {
         tmp=list_for_subgraph->next;
         /* Before exploring an element that points on a subgraph match,
          * we decrease its 'pointed_by' variable that was previously increased
          * in the 'add_match_in_list' function */
         (list_for_subgraph->match->pointed_by)--;
         explore_tfst(tfst,list_for_subgraph->match->dest_state_text,
                           grammar_transition->state_number,
                           depth,list_for_subgraph->match,LIST,infos,-1,NULL);
         /* Finally, we remove, if necessary, the list of match element
          * that was used for storing the subgraph match. This cleaning
          * will only free elements that are not involved in others
          * matches, that is to say element with pointed_by=0 */
         clean_tfst_match_list(list_for_subgraph->match,match_element_list);
         free(list_for_subgraph);
         list_for_subgraph=tmp;
      }
   }
   else {
      /* Normal case (not a subgraph call) */
      struct tfst_match* list=NULL;
      Transition* text_transition=tfst->automaton->states[current_state_in_tfst]->outgoing_transitions;
      /* For a given tag in the grammar, we test all the transitions in the
       * text automaton, and we note all the states we can reach */
      while (text_transition!=NULL) {
         int pos_kr=-1;
         int result=match_between_text_and_grammar_tags(tfst,(TfstTag*)(tfst->tags->tab[text_transition->tag_number]),
                                                 infos->fst2->tags[grammar_transition->tag_number],
                                                 text_transition->tag_number,
                                                 grammar_transition->tag_number,
                                                 infos,&pos_kr);
         if (result==OK_MATCH_STATUS) {
            /* Case of a match with something in the text automaton (i.e. <V>) */
            list=insert_in_tfst_matches(list,current_state_in_tfst,text_transition->state_number,
                 grammar_transition,pos_kr,text_transition->tag_number);
         }
         else if (result==TEXT_INDEPENDENT_MATCH) {
            /* Case of a match independent of the text automaton (i.e. <E>) */
            list=insert_in_tfst_matches(list,current_state_in_tfst,current_state_in_tfst,
                 grammar_transition,-1,NO_TEXT_TOKEN_WAS_MATCHED);
         }
         text_transition=text_transition->next;
      }
      struct tfst_match* tmp;
      /* Then, we continue the exploration from the reached states. This
       * procedure avoids exploring several times a same state when
       * it can be reached through various tags in the text automaton.
       * For instance, if we have "le" in the grammar and {le,.DET} and
       * {le,.PRO} in the text automaton that point to the same state XXX,
       * we will explore XXX just once. */
      while (list!=NULL) {
         tmp=list->next;
         list->next=match_element_list;
         /* match_element_list is pointed by one more element */
         if (match_element_list!=NULL) {
        	 (match_element_list->pointed_by)++;
         }
         Transition* tmp_trans=(list->pos_kr!=-1)?list->fst2_transition:NULL;
         int dest_state=(list->pos_kr!=-1)?-1:grammar_transition->state_number;
         explore_tfst(tfst,list->dest_state_text,dest_state,
                           depth,list,LIST,infos,list->pos_kr,tmp_trans);
         if (list->pointed_by==0) {
            /* If list is not blocked by being part of a match for the calling
             * graph, we can free it */
            list->next=NULL;
            if (match_element_list!=NULL) {(match_element_list->pointed_by)--;}
            free_tfst_match(list);
         }
         list=tmp;
      }
   }
   grammar_transition=grammar_transition->next;
}
}



/**
 * This function tests if a text tag can be matched by a grammar tag.
 */
int match_between_text_and_grammar_tags(Tfst* tfst,TfstTag* text_tag,Fst2Tag grammar_tag,
                                        int tfst_tag_index,int fst2_tag_index,
                                        struct locate_tfst_infos* infos,int *pos_kr) {
if (grammar_tag->type==BEGIN_POSITIVE_CONTEXT_TAG
	|| grammar_tag->type==BEGIN_NEGATIVE_CONTEXT_TAG
	|| grammar_tag->type==END_CONTEXT_TAG
	|| grammar_tag->type==LEFT_CONTEXT_TAG
	|| grammar_tag->type==BEGIN_MORPHO_TAG
	|| grammar_tag->type==END_MORPHO_TAG) {
   fatal_error("Tag '%S' should not be found in a grammar applied to a text automaton\n",grammar_tag->input);
}
/**************************************************
 * We want to match something that is text independent */
if (grammar_tag->type==BEGIN_VAR_TAG
	|| grammar_tag->type==END_VAR_TAG
    || !u_strcmp(grammar_tag->input,"<E>")) {
   return TEXT_INDEPENDENT_MATCH;
}
/* Here we test the special case of the " " and # tags */
if (!u_strcmp(grammar_tag->input," ")) {
	if (is_space_on_the_left_in_tfst(tfst,text_tag)) {
		return TEXT_INDEPENDENT_MATCH;
	}
	return NO_MATCH_STATUS;
}
if (!u_strcmp(grammar_tag->input,"#")) {
   if (!is_space_on_the_left_in_tfst(tfst,text_tag)) {
		return TEXT_INDEPENDENT_MATCH;
	}
	return NO_MATCH_STATUS;
}

/* Now, we will compare the text tag and the grammar one */

if (!u_strcmp(text_tag->content,"{STOP}")) {
	/* {STOP} can NEVER be matched */
	return NO_MATCH_STATUS;
}


if (infos->korean && (*pos_kr!=-1 || (grammar_tag->input[0]!='{' && grammar_tag->input[0]!='<'))) {
   /* If we have a Korean token in the fst2 */
   if (*pos_kr==-1) {
      /* If we were not in token exploration mode, we turn into this mode now */
      *pos_kr=0;
   }
   unichar* jamo_tfst=infos->jamo_tfst_tags[tfst_tag_index];
   unichar* jamo_fst2=infos->jamo_fst2_tags[fst2_tag_index];
   int k=(*pos_kr);
   int j=0;
   while (jamo_fst2[k]!='\0' && jamo_tfst[j]!='\0') {
      /* We ignore syllab bounds n both tfst and fst2 tags */
      if (jamo_fst2[k]==KR_SYLLAB_BOUND) {
         k++;
         continue;
      }
      if (jamo_tfst[j]==KR_SYLLAB_BOUND) {
         j++;
         continue;
      }
      if (jamo_fst2[k]!=jamo_tfst[j]) {
         /* If a character doesn't match */
         return NO_MATCH_STATUS;
      }
      k++;
      j++;
   }
   if (jamo_fst2[k]=='\0' && jamo_tfst[j]=='\0') {
      /* If we are at both ends of strings, it's a full match */
      (*pos_kr)=-1;
      return OK_MATCH_STATUS;
   }
   if (jamo_fst2[k]=='\0') {
      /* If we are at the end of the fst2 tag but not at the end of the tfst tag, we fail */
      return NO_MATCH_STATUS;
   }
   /* If we have consumed all the tfst tag, but not all th fst2 one, it's a partial match */
   (*pos_kr)=k;
   error("partial match between tfst=%S and fst2=%S\n",jamo_tfst,jamo_fst2);
   return OK_MATCH_STATUS;
}


struct dela_entry* grammar_entry=NULL;
struct dela_entry* text_entry=NULL;
/**************************************************
 * We want to match a token like "le" */
if (is_letter(grammar_tag->input[0],infos->alphabet)) {
   if (is_letter(text_tag->content[0],infos->alphabet)) {
      /* text= "toto" */
      if (grammar_tag->control & RESPECT_CASE_TAG_BIT_MASK) {
         /* If we must respect case */
         if (!u_strcmp(grammar_tag->input,text_tag->content)) {return OK_MATCH_STATUS;}
         else {return NO_MATCH_STATUS;}
      } else {
         /* If case does not matter */
         if (is_equal_or_uppercase(grammar_tag->input,text_tag->content,infos->alphabet)) {return OK_MATCH_STATUS;}
         else {return NO_MATCH_STATUS;}
      }
   } else if (text_tag->content[0]=='{' && text_tag->content[1]!='\0') {
      /* text={toto,tutu.XXX} */
	  text_entry=tokenize_tag_token(text_tag->content);
	  if (text_entry==NULL) {
		  fatal_error("NULL text_entry error in match_between_text_and_grammar_tags\n");
	  }
	  if (grammar_tag->control & RESPECT_CASE_TAG_BIT_MASK) {
         /* If we must respect case */
		 if (!u_strcmp(grammar_tag->input,text_entry->inflected)) {
			 goto ok_match;
		 } else {
			 goto no_match;
		 }
      } else {
         /* If case does not matter */
    	 if (is_equal_or_uppercase(grammar_tag->input,text_entry->inflected,infos->alphabet)) {
    		 goto ok_match;
    	 } else {
    		 goto no_match;
    	 }
      }
   }
   return NO_MATCH_STATUS;
}

/**************************************************
 * We want to match a tag like "{tutu,toto.XXX}" */
if (grammar_tag->input[0]=='{' && u_strcmp(grammar_tag->input,"{S}")) {
   if (text_tag->content[0]!='{') {
      /* If the text tag is not of the form "{tutu,toto.XXX}" */
      return NO_MATCH_STATUS;
   }
   grammar_entry=tokenize_tag_token(grammar_tag->input);
   text_entry=tokenize_tag_token(text_tag->content);
   if (!is_equal_or_uppercase(grammar_entry->inflected,text_entry->inflected,infos->alphabet)) {
      /* We allow case variations on the inflected form :
       * if there is "{tutu,toto.XXX}" in the grammar, we want it
       * to match "{Tutu,toto.XXX}" in the text automaton */
	  goto no_match;
   }
   if (u_strcmp(grammar_entry->lemma,text_entry->lemma)) {
      /* If lemmas are different,we don't match */
	  goto no_match;
   }
   if (!same_codes(grammar_entry,text_entry)) {
      /* If grammatical, semantical and inflectional informations
       * are different we don't match*/
	  goto no_match;
   }
   goto ok_match;
}

/**************************************************
 * We want to match something like "<....>". The
 * <E> case has already been handled in text independent matchings */
if (grammar_tag->input[0]=='<' && grammar_tag->input[1]!='\0') {
	/* We tokenize the text tag, if we have one */
	if (text_tag->content[0]=='{' && text_tag->content[1]!='\0') {
	   text_entry=tokenize_tag_token(text_tag->content);
	}
   if (!u_strcmp(grammar_tag->input,"<MOT>")) {
      /* <MOT> matches a sequence of letters or a tag like {tutu,toto.XXX}, even
       * if 'tutu' is not made of characters */
      if (is_letter(text_tag->content[0],infos->alphabet) || text_entry!=NULL) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<!MOT>")) {
      /* <!MOT> matches the opposite of <MOT> */
      if (!is_letter(text_tag->content[0],infos->alphabet)
          && text_entry==NULL) {
    	  goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<MIN>")) {
      /* <MIN> matches a sequence of letters or a tag like {tutu,toto.XXX}
       * only made of lower case letters */
      if (is_sequence_of_lowercase_letters(text_tag->content,infos->alphabet) ||
          (text_entry!=NULL && is_sequence_of_lowercase_letters(text_entry->inflected,infos->alphabet))) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<!MIN>")) {
      /* <!MIN> matches a sequence of letters or a tag like {tutu,toto.XXX}
       * that is not only made of lower case letters */
      if ((is_letter(text_tag->content[0],infos->alphabet) && !is_sequence_of_lowercase_letters(text_tag->content,infos->alphabet))
          || (text_entry!=NULL && !is_sequence_of_lowercase_letters(text_entry->inflected,infos->alphabet))) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<MAJ>")) {
      /* <MAJ> matches a sequence of letters or a tag like {TUTU,toto.XXX}
       * only made of upper case letters */
      if (is_sequence_of_uppercase_letters(text_tag->content,infos->alphabet) ||
          (text_entry!=NULL && is_sequence_of_uppercase_letters(text_entry->inflected,infos->alphabet))) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<!MAJ>")) {
      /* <!MAJ> matches a sequence of letters or a tag like {tutu,toto.XXX}
       * that is not only made of upper case letters */
      if ((is_letter(text_tag->content[0],infos->alphabet) && !is_sequence_of_uppercase_letters(text_tag->content,infos->alphabet))
          || (text_entry!=NULL && !is_sequence_of_uppercase_letters(text_entry->inflected,infos->alphabet))) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<PRE>")) {
      /* <PRE> matches a sequence of letters or a tag like {TUTU,toto.XXX}
       * that begins by an upper case letter */
      if (is_upper(text_tag->content[0],infos->alphabet) ||
          (text_entry!=NULL && is_upper(text_entry->inflected[0],infos->alphabet))) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<!PRE>")) {
      /* <!PRE> matches a sequence of letters or a tag like {TUTU,toto.XXX}
       * that does not begin by an upper case letter */
      if ((is_letter(text_tag->content[0],infos->alphabet) && !is_upper(text_tag->content[0],infos->alphabet)) ||
          (text_entry!=NULL && is_letter(text_entry->inflected[0],infos->alphabet) && !is_upper(text_entry->inflected[0],infos->alphabet))) {
         /* We test is_letter+!is_upper instead of is_lower in order to handle
          * cases where there is no difference between lower and upper case letters
          * (for instance Thai) */
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<TOKEN>")) {
      /* <TOKEN> matches anything but the {STOP} tag, and the {STOP} tag case
       * has already been dealt with */
      goto ok_match;
   }
   if (!u_strcmp(grammar_tag->input,"<DIC>")) {
      /* <DIC> matches any tag like {tutu,toto.XXX} */
      if (text_entry!=NULL) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<!DIC>")) {
      /* <DIC> matches any sequence of letters that is not a tag like {tutu,toto.XXX} */
      if (is_letter(text_tag->content[0],infos->alphabet)) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<SDIC>")) {
      /* <SDIC> matches any tag like {tutu,toto.XXX} where tutu is simple word
       *
       * NOTE: according to the formal definition of simple words, something like
       *       {3,.NUMBER} is not considered as a simple word since 3 is not
       *       a letter. It will be considered as a compound word */
      if (text_entry!=NULL && is_sequence_of_letters(text_entry->inflected,infos->alphabet)) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<CDIC>")) {
      /* <CDIC> matches any tag like {tutu,toto.XXX} where tutu is a compound word
       *
       * NOTE: see <SDIC> note */
      if (text_entry!=NULL && !is_sequence_of_letters(text_entry->inflected,infos->alphabet)) {
         goto ok_match;
      }
      goto no_match;
   }
   if (!u_strcmp(grammar_tag->input,"<NB>")) {
      /* <NB> matches any tag like 3 or {37,toto.XXX} */
      if (u_are_digits(text_tag->content) ||
          (text_entry!=NULL && u_are_digits(text_entry->inflected))) {
         goto ok_match;
      }
      goto no_match;
   }
   /**************************************************************
    * If we arrive here, we are in the case of a pattern.
    * We will handle several cases, but only if the text contains
    * a tag like {tutu,toto.XXX} */
   if (text_entry==NULL) {
      goto no_match;
   }
   /* First, we tokenize the grammar pattern tag */
   int negation;
   struct pattern* pattern=tokenize_grammar_tag(grammar_tag->input,&negation);
   int ok=is_entry_compatible_with_pattern(text_entry,pattern);
   free_pattern(pattern);
   if ((ok && !negation) || (!ok && negation)) {
	   goto ok_match;
   }
   goto no_match;
}

/**************************************************
 * Last case: we want to match a non alphabetic token like "!" */
if (!u_strcmp(grammar_tag->input,text_tag->content)) {
   return OK_MATCH_STATUS;
}
return NO_MATCH_STATUS;

/* We arrive here when we have used dela_entry variables */
int ret_value;

ok_match:
/* We test the morphological filter, if any */
if (!morphological_filter_is_ok((text_entry!=NULL)?text_entry->inflected:text_tag->content,grammar_tag,infos)) {
	goto no_match;
}
ret_value=OK_MATCH_STATUS;
goto clean;

no_match:
ret_value=NO_MATCH_STATUS;
/* This goto is useless, but there because if one day someone inserts something here...
 * Note that our friend compiler certainly removes it */
goto clean;

clean:
free_dela_entry(grammar_entry);
free_dela_entry(text_entry);
return ret_value;
}


/**
 * This function takes a tag of the form <.......> and tokenizes it.
 */
struct pattern* tokenize_grammar_tag(unichar* tag,int *negation) {
(*negation)=0;
if (tag==NULL) {
   error("NULL pattern error in tokenize_grammar_tag\n");
   return NULL;
}
if (tag[0]!='<') {
   error("ERROR: a pattern does not start with < in tokenize_grammar_tag\n");
   return NULL;
}
int l=u_strlen(tag);
if (tag[l-1]!='>') {
   error("ERROR: a pattern does not end with > in tokenize_grammar_tag\n");
   return NULL;
}
if (tag[1]=='!') {(*negation)=1;}
else {(*negation)=0;}
tag[l-1]='\0';
struct pattern* pattern=build_pattern(&(tag[1+(*negation)]),NULL);
tag[l-1]='>';
return pattern;
}


/**
 * Returns 1 if there is a space on the immediate left of the given tag.
 */
int is_space_on_the_left_in_tfst(Tfst* tfst,TfstTag* tag) {
if (tag->start_pos_char==0) {
	/* The tag starts exactly at the beginning of a token, so we just
	 * have to look if the previous token was ending with a space. By convention,
	 * we say that the token #0 has no space on its left */
	if (tag->start_pos_token==0) {
		return 0;
	}
	int size_of_previous=tfst->token_sizes->tab[tag->start_pos_token-1];
	return tfst->token_content[tag->start_pos_token-1][size_of_previous-1]==' ';
} else {
	/* The tag is in the middle of a token */
	return tfst->token_content[tag->start_pos_token][tag->start_pos_char-1]==' ';
}
}


/**
 * Tests if the text tag is compatible with the grammar tag morphological filter, if any.
 * Returns 1 in case of success (no filter is a case of success); 0 otherwise.
 */
int morphological_filter_is_ok(unichar* content,Fst2Tag grammar_tag,struct locate_tfst_infos* infos) {
if (grammar_tag->filter_number==-1) {
	return 1;
}
return string_match_filter(infos->filters,content,grammar_tag->filter_number);
}
