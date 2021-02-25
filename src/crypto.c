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
struct T_SearchResult * results;


/**
@brief Appends a new result
@description Insert a new element in the result linked list ordered by algorithm name. If the algorithm already exists, the result is discarded
@param element Structure that contains an existing algorithm leaf
*/


int appendToResults(struct T_TrieNode *element){

struct T_SearchResult* temp = (struct T_SearchResult*) calloc(1,sizeof(struct T_SearchResult));

temp->element= element;
temp->nextElement=NULL;

struct T_SearchResult* temp2 = results;
struct T_SearchResult** temp3 = &results;
int res =0;
while((temp2 != NULL) && (res<1) )
{
	if(temp->element==NULL) return 0; 
	if(temp2->element->algorithmName==NULL) return 0;
	res = strcmp(temp2->element->algorithmName,temp->element->algorithmName);
	if(res==0) return 0;
	

   temp3 = &temp2->nextElement;
   temp2 = temp2->nextElement;
  
}

*temp3 = temp;

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

     root=(struct T_TrieNode *) calloc (1,sizeof (struct T_TrieNode));
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
	aux =(char *) calloc(1,endToken-startToken+1);

	memcpy(aux,&text[startToken],endToken-startToken);
	aux[endToken-startToken]='\0';
	*start=startToken;
	*end=endToken;
	return aux;

}

/**
@description Gets a token from a valid portion of a char array 
@param	text The text to search from
@param	start Begining of search area. If a valid token is found, the effective begining is returned;
@param	end Effective index of token end.
@param	maxLen Bounds the search to a limited index 
	
*/
void clean_crypto_definitions(void)
{
//printf("cleaning\r\n");
cleanCrypto(root);
//printf("cleaned\r\n");

}


/**
@description Mines a given path that contains MD5 Files. 
@param	mined_path 
@param	md5 name of file to be mined.
@param	src The contents of the MD5 file.
@param	src_ln Size of the buffer to be mined (equal to filesize)
@since 2.1.2	
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
	//printf("Procesando %s ",md5);	
	if (mined_path)
	{	dumpToFile=1;
		strcpy(csv_path, mined_path);
		strcat(csv_path, "/crypto.csv");
	}
	 FILE * fp;
	 
	if (dumpToFile) {
		fp = fopen(csv_path, "a");
		dumpToFile=1;
	}
	results=NULL;
	
	while(start<src_ln){
		auxLn=getNextToken(src,&start, &end,src_ln);
		if((start==end)) 
			break;
		 else {
			start=end;
			if((auxLn!=NULL)&&(strlen(auxLn)>2)){
			toLower(auxLn);
			
			struct T_TrieNode * nodo = searchAlgorithm(auxLn, root) ;
			if(nodo!=NULL && nodo->algorithmName!=NULL){
				appendToResults(nodo);
				
			}
			}
		}
		if(auxLn!=NULL) free(auxLn);
	}
	
	struct T_SearchResult * aux=results; 
 	struct T_SearchResult * old=aux;
	
	while(aux!=NULL){
	old=aux;
	if(dumpToFile) 
		fprintf(fp,"%s,%s,%d\r\n",md5,
					aux->element->algorithmName,
					aux->element->coding); 
	else {
		printf("%s,%s,%d\r\n",md5,
					aux->element->algorithmName,
					aux->element->coding);
					 }	
	aux=aux->nextElement;
	free(old);
	}
		
	if (dumpToFile)
		fclose(fp);
			
	
	
}
