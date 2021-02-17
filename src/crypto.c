// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/crypto.c
 *
 * SCANOSS License detection subroutines
 *
 * Copyright (C) 2018-2020 SCANOSS.COM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <libgen.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include "minr.h"
#include "crypto.h"
#include "trie.h"
#include <string.h>
/** Definition for forward recursive reference*/
struct T_SearchResult;

struct T_SearchResult{
	struct T_TrieNode *element;
	struct T_SearchResult *nextElement;
};
/**/
struct T_TrieNode * root;
//struct T_SearchResult * results;
struct T_TrieNode * results[100];
unsigned int resultsCount=0;
/*
int appendToResultsOld(struct T_TrieNode *element){

struct T_SearchResult *auxFirst=results;

struct T_SearchResult *current=auxFirst;

struct T_SearchResult *last =auxFirst;

int res =-1;
while((current!=NULL)){
	res= strcmp(current->element->algorithmName,element->algorithmName);
	if(res==0) return 0; 
	if(res<0) {
		last=current;
		current=current->nextElement;
			
	} 
	if(res>0) break;
}
struct T_SearchResult *link;
link = (struct T_SearchResult*) malloc(sizeof(struct T_SearchResult));
	link->element=element;	
	link->nextElement=current;
	if(results==NULL) 
		results= link; 
	if(last!=NULL)
		last->nextElement=link;
	
		
	
return 1;
}*/

int res=-1;
int appendToResults(struct T_TrieNode *element){
int actual = 0;
while(actual<resultsCount){
	res= strcmp(results[actual]->algorithmName,element->algorithmName);
	if(res==0) return 0; 
	if(res<0) actual++;
	if(res>0) break;
}
for(int i=resultsCount;i>actual;i--)
	results[i]=results[i-1];
results[actual]=element;
resultsCount++;
return 1;
}






bool is_file(char *path);
bool is_dir(char *path);

bool not_a_dot(char *path);
/**
@brief Load algorithms definitions
@description Loads alorithms definitions from the default directory
*/
void load_crypto_definitions(void){

     root=(struct T_TrieNode *) malloc (sizeof (struct T_TrieNode));
     parseDirectory("./crypto");
 
}
/**
@description Gets a token from a valid portion of a char array 
@param	text The text to search from
@param	start Begining of search area. If a valid token is found, the effective begining is returned;
@param	end Effective index of token end.
@param	maxLen Bounds the search to a limited index 
	
*/


char *getNextToken(char * text,uint64_t *start,uint64_t *end,uint64_t maxLen){
	uint64_t startToken=*start;
	uint64_t endToken=0;
	char *aux;
	while((startToken<maxLen)&&(indexOf(text[startToken])==-1))
		startToken++;
	endToken=startToken;
	while((endToken<maxLen)&&(indexOf(text[endToken])!=-1))
		endToken++;
		
	if(startToken==endToken){
	*start=startToken;
	*end=endToken;
	return NULL;
	}
	aux =(char *) malloc(endToken-startToken+1);

	memcpy(aux,&text[startToken],endToken-startToken);
	aux[endToken-startToken]='\0';
	*start=startToken;
	*end=endToken;
	return aux;

}
/*
	 */
void mine_crypto(char *mined_path, char *md5, char *src, uint64_t src_ln)
{
	//FILE *fp;
	char *auxLn;
	uint64_t start = 0;
	uint64_t end = 0;
	/* Assemble csv path */
	char csv_path[MAX_PATH_LEN] = "\0";
	char dumpToFile=0;
	
	if (mined_path)
	{	dumpToFile=1;
		strcpy(csv_path, mined_path);
		strcat(csv_path, "/crypto.csv");
	}
	 FILE * fp;
	 resultsCount = 0;
	
	if (dumpToFile) {
		fp = fopen(csv_path, "a");
		dumpToFile=1;
	}
			
	while(start<src_ln){
		auxLn=getNextToken(src,&start, &end,src_ln);
		if(start==end) 
			break;
		 else {
			start=end;
			toLower(auxLn);
			
			struct T_TrieNode * nodo = searchAlgorithm(auxLn, root) ;
			if(nodo!=NULL){
				appendToResults(nodo);
			}
		}
	}
		
for(int i=0;i<resultsCount;i++){
	if(dumpToFile) 
		fprintf(fp,"%s,%s,%d\r\n",md5,
					results[i]->algorithmName,
					results[i]->coding); 
	else {
		printf("%s,%s,%d\r\n",md5,
					results[i]->algorithmName,
					results[i]->coding);
					 }	
	
	}
	
	if (dumpToFile)
		fclose(fp);
	
}
