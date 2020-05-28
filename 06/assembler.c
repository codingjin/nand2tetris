#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRLEN 256
#define MAX_LINENO 60000
#define MAX_NAMELEN 32
#define TABLESIZE 997
#define SHIFT 4
#define BINLEN 16

char codeline[MAX_LINENO][MAX_STRLEN];
char bincode[BINLEN+2];// bincode\n0
char comp[MAX_STRLEN], dest[MAX_STRLEN], jump[MAX_STRLEN];

typedef struct Node{
	char key[MAX_NAMELEN];
	int value;
	struct Node *next;
}Node;

Node *symtable[TABLESIZE];
int hash(char *str);
void insert(char *key, int value);
Node *lookup(char *key);
void init_symtable();
void print_symtable();
int is_digit(char c);

void parse_dest(char *dest, char *bincode, char *codeline);
void parse_jump(char *jump, char *bincode, char *codeline);
void parse_comp(char *comp, char *bincode, char *codeline);


int main(int argc, char **argv)
{
	size_t linestrlen = MAX_STRLEN;
	char *linestr = (char*)malloc(linestrlen);
	if (!linestr) {
		printf("failed to malloc for linestr!\n");
		return 0;
	}

	for (int i=0;i<MAX_LINENO;++i)
		for (int j=0;j<MAX_STRLEN;++j)
			codeline[i][j] = 0;

	init_symtable();  //print_symtable();

	int linecounter = 0;
	int codeindex = 0;
	int offset = 16;
	int len, num;
	char tempstr[MAX_STRLEN];

	while (getline(&linestr, &linestrlen, stdin) >= 0) {
		if (linestr[0] == '\n')	continue;
		//printf("%s len=%d\n", linestr, strlen(linestr));
		else if (linestr[0] == '(') {  // JUMP LABEL
			for (int i=1;i<MAX_STRLEN;++i)
				if (linestr[i] == ')') {
					len = i-1;
					strncpy(tempstr, linestr+1, len);
					tempstr[len] = 0;
					insert(tempstr, codeindex);
					break;
				}
		}else {
			++linecounter;
			len = strlen(linestr);
			--len;
			strncpy(codeline[codeindex], linestr, len);
			codeline[codeindex][len] = 0;
			++codeindex;
		}
	}
	free(linestr);

	/*
	// printout codeline
	for (int i=0;i<linecounter;++i)	printf("%d:%s\n", i, codeline[i]);
	print_symtable();
	*/

	// output the binary code
	for (int i=0;i<linecounter;++i) {
		if (codeline[i][0] == '@') { // A instruction
			memset(bincode, '0', BINLEN);
			bincode[BINLEN] = '\n';
			bincode[BINLEN+1] = 0;
			if (is_digit(codeline[i][1])) { // @number
				num = atoi(codeline[i]+1);
				if (num > 0) {
					int j = 15;
					while (num) {
						bincode[j--] += num&1;
						num >>= 1;
						if (j == -1) {
							fprintf(stderr, "invalid number in @number code\n");
							exit(0);
						}
					}
				}
			}else { // @name
				Node *node = lookup(codeline[i]+1);
				if (!node) { // it is a variable declaration
					insert(codeline[i]+1, offset++);
					num = offset - 1;
				}else	num = node->value;

				if (num > 0) {
					int j = 15;
					while (num) {
						bincode[j--] += num&1;
						num >>= 1;
						if (j == -1) {
							fprintf(stderr, "invalid number for variable[%s]\n", codeline[i]+1);
							exit(0);
						}
					}
				}
			}

		}else { // C instruction
			bincode[0] = '1';
			bincode[1] = '1';
			bincode[2] = '1';
			memset(bincode+3, '0', 13);
			bincode[BINLEN] = '\n';
			bincode[BINLEN+1] = 0;

			int pos1=0, pos2=0;
			for (int j=1;j<MAX_STRLEN;++j) {
				if (codeline[i][j] == 0) break;
				else if (codeline[i][j] == '=') pos1=j;
				else if (codeline[i][j] == ';') pos2=j;
			}

			if (pos1>0 && pos2>0) { // dest=comp;jump
				if (pos1 >= pos2) {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline[i]);
					exit(0);
				}
				strncpy(tempstr, codeline[i], pos1);
				tempstr[pos1] = 0;
				parse_dest(tempstr, bincode, codeline[i]);

				strcpy(tempstr, codeline[i]+pos2);
				parse_jump(tempstr, bincode, codeline[i]);

				strncpy(tempstr, codeline[i]+pos1+1, pos2-pos1-1);
				tempstr[pos2-pos1-1] = 0;
				parse_comp(tempstr, bincode, codeline[i]);

			}else if (pos1 > 0) { // dest=comp
				strncpy(tempstr, codeline[i], pos1);
				tempstr[pos1] = 0;
				parse_dest(tempstr, bincode, codeline[i]);

				strcpy(tempstr, codeline[i]+pos1+1);
				parse_comp(tempstr, bincode, codeline[i]);

			}else if (pos2 > 0) { // comp;jump
				strcpy(tempstr, codeline[i]+pos2+1);
				parse_jump(tempstr, bincode, codeline[i]);

				strncpy(tempstr, codeline[i], pos2);
				tempstr[pos2] = 0;
				parse_comp(tempstr, bincode, codeline[i]);

			}else { // comp
				strcpy(tempstr, codeline[i]);
				parse_comp(tempstr, bincode, codeline[i]);
			}
		}
		printf("%s", bincode);
	} // end of pass of one codeline

	return 0;
}

int hash(char *str)
{
	if (!str || strlen(str)==0) {
		fprintf(stderr, "invalid str calls hash()\n");
		exit(0);
	}

	int hashcode = 0;
	int len = strlen(str);
	for (int i=0;i<len;++i)
		hashcode = ((hashcode<<SHIFT) + str[i]) % TABLESIZE;
	return hashcode;
}

void insert(char *key, int value)
{
	if (!key || strlen(key)==0 || value<0) {
		fprintf(stderr, "invalid key-value calls inser()\n");
		exit(0);
	}

	if (lookup(key)) {
		fprintf(stderr, "insert an already existed key into the symtable\n");
		exit(0);
	}

	int hashcode = hash(key);
	int index = hashcode>=0? hashcode:-hashcode;

	Node *newnode = (Node*)calloc(1, sizeof(Node));
	strcpy(newnode->key, key);
	newnode->value = value;

	if (!symtable[index]) symtable[index]=newnode;
	else {
		Node *node = symtable[index]->next;
		symtable[index]->next = newnode;
		newnode->next = node;
	}
}

Node *lookup(char *key)
{
	if (!key || strlen(key)==0) {
		fprintf(stderr, "invalid key calls lookup()\n");
		exit(0);
	}

	int hashcode = hash(key);
	int index = hashcode>=0? hashcode:-hashcode;

	if (!symtable[index])	return NULL;

	Node *node = symtable[index];
	while (node) {
		if(strcmp(node->key, key)==0)	return node;
		else	node = node->next;
	}
	return NULL;
}

void init_symtable()
{
	for (int i=0;i<TABLESIZE;++i)	symtable[i]=0;

	insert("R0", 0);
	insert("R1", 1);
	insert("R2", 2);
	insert("R3", 3);
	insert("R4", 4);
	insert("R5", 5);
	insert("R6", 6);
	insert("R7", 7);
	insert("R8", 8);
	insert("R9", 9);
	insert("R10", 10);
	insert("R11", 11);
	insert("R12", 12);
	insert("R13", 13);
	insert("R14", 14);
	insert("R15", 15);
	insert("SCREEN", 16384);
	insert("KBD", 24576);
	insert("SP", 0);
	insert("LCL", 1);
	insert("ARG", 2);
	insert("THIS", 3);
	insert("THAT", 4);
}

void print_symtable()
{
	Node *node;
	for (int i=0;i<TABLESIZE;++i) {
		if (symtable[i]) {
			node = symtable[i];
			while (node) {
				printf("key[%s]value[%d]\n", node->key, node->value);
				node = node->next;
			}
		}
	}
}

int is_digit(char c)
{
	return c>=48 && c<=57;
}


// dest bincode[10,11,12]
void parse_dest(char *dest, char *bincode, char *codeline)
{
	int len = strlen(dest);
	if (!dest || !len) {
		fprintf(stderr, "invalid dest calls parse_dest, code[%s]\n", codeline);
		exit(0);
	}

	if (len == 1) {
		if (dest[0] == 'M')	bincode[12] = '1';
		else if (dest[0] == 'D')	bincode[11] = '1';
		else if (dest[0] == 'A')	bincode[10] = '1';
		else {
			fprintf(stderr, "invalid C instruction[%s]\n", codeline);
			exit(0);
		}
	}else if (len == 2) {
		if (dest[0] == 'M') {
			if (dest[1] != 'D') {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}
			bincode[11] = '1';
			bincode[12] = '1';
		}else {
			if (dest[0] != 'A') {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}
			bincode[10] = '1';
			if (dest[1] == 'M')	bincode[12] = '1';
			else if (dest[1] == 'D')	bincode[11] = '1';
			else {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}
		}
	}else if (len == 3)
		memset(bincode+10, '1', 3);
	else {
		fprintf(stderr, "invalid C instruction[%s]\n", codeline);
		exit(0);
	}
}

// jump bincode[13,14,15]
void parse_jump(char *jump, char *bincode, char *codeline)
{
	if (strlen(jump)!=3 || jump[0]!='J') {
		fprintf(stderr, "invalid C instruction[%s]\n", codeline);
		exit(0);
	}

	if (jump[1] == 'G') {
		bincode[15] = '1';
		if (jump[2] == 'E')	bincode[14] = '1';
	}else if (jump[1] == 'E') {
		bincode[14] = '1';
	}else if (jump[1] == 'L') {
		bincode[13] = '1';
		if (jump[2] == 'E')	bincode[14] = '1';
	}else if (jump[1] == 'N') {
		bincode[13] = '1';
		bincode[15] = '1';
	}else	memset(bincode+13, '1', 3);

}

// comp bincode[3,4,...,9]
void parse_comp(char *comp, char *bincode, char *codeline)
{
	if (!comp) {
		fprintf(stderr, "null comp calls parse_comp, invalid C instruction[%s]\n", codeline);
		exit(0);
	}
	int len =  strlen(comp);
	if (len == 1) {
		if (comp[0] == '0')	strncpy(bincode+3, "0101010", 7);
		else if (comp[0] == '1') strncpy(bincode+3, "0111111", 7);
		else if (comp[0] == 'D') strncpy(bincode+3, "0001100", 7);
		else if (comp[0] == 'A') strncpy(bincode+3, "0110000", 7);
		else if (comp[0] == 'M') strncpy(bincode+3, "1110000", 7);
		else {
			fprintf(stderr, "invalid C instruction[%s]\n", codeline);
			exit(0);
		}
	}else if (len == 2) {
		if (comp[0] == '-') {
			if (comp[1] == '1')	strncpy(bincode+3, "0111010", 7);
			else if (comp[1] == 'D')	strncpy(bincode+3, "0001111", 7);
			else if (comp[1] == 'A')	strncpy(bincode+3, "0110011", 7);
			else if (comp[1] == 'M')	strncpy(bincode+3, "1110011", 7);
			else {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}
		}else if (comp[0] == '!') {
			if (comp[1] == 'D')	strncpy(bincode+3, "0001101", 7);
			else if (comp[1] == 'M') strncpy(bincode+3, "1110001", 7);
			else if (comp[1] == 'A') strncpy(bincode+3, "0110001", 7);
			else {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}
		}else {
			fprintf(stderr, "invalid C instruction[%s]\n", codeline);
			exit(0);
		}
	}else if (len == 3) {
		if (comp[0] == 'D') { // D+1, D-1, D+A and D+M, D-A and D-M, D&A and D&M, D|A and D|M 
			if (comp[2] == '1') { // D+1 and D-1
				if (comp[1] == '+')	strncpy(bincode+3, "0011111", 7);
				else if (comp[1] == '-')	strncpy(bincode+3, "0001110", 7);
				else {
					fprintf(stderr, "invalid instruction[%s]\n", codeline);
					exit(0);
				}
			}else if (comp[1] == '+') { // D+A and D+M
				if (comp[2] == 'A')	strncpy(bincode+3, "0000010", 7);
				else if (comp[2] == 'M')	strncpy(bincode+3, "1000010", 7);
				else {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline);
					exit(0);
				}
			}else if (comp[1] == '-') { // D-A and D-M
				if (comp[2] == 'A')	strncpy(bincode+3, "0010011", 7);
				else if (comp[2] == 'M')	strncpy(bincode+3, "1010011", 7);
				else {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline);
					exit(0);
				}
			}else if (comp[1] == '&') { // D&A and D&M
				if (comp[2] == 'A')	strncpy(bincode+3, "0000000", 7);
				else if (comp[2] == 'M')	strncpy(bincode+3, "1000000", 7);
				else {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline);
					exit(0);
				}
			}else if (comp[1] == '|') { // D|A and D|M
				if (comp[2] == 'A')	strncpy(bincode+3, "0010101", 7);
				else if (comp[2] == 'M')	strncpy(bincode+3, "1010101", 7);
				else {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline);
					exit(0);
				}
			}else {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}

		}else if (comp[0] == 'A') { // A+1, A-1, A-D
			if (comp[1] == '+')
				strncpy(bincode+3, "0110111", 7);
			else if (comp[1] == '-') {
				if (comp[2] == '1')	strncpy(bincode+3, "0110010", 7);
				else if (comp[2] == 'D')	strncpy(bincode+3, "0000111", 7);
				else {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline);
					exit(0);
				}
			}else {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}

		}else if (comp[0] == 'M') { // M+1, M-1, M-D
			if (comp[1] == '+')
				strncpy(bincode+3, "1110111", 7);
			else if (comp[1] == '-') {
				if (comp[2] == '1')	strncpy(bincode+3, "1110010", 7);
				else if (comp[2] == 'D')	strncpy(bincode+3, "1000111", 7);
				else {
					fprintf(stderr, "invalid C instruction[%s]\n", codeline);
					exit(0);
				}

			}else {
				fprintf(stderr, "invalid C instruction[%s]\n", codeline);
				exit(0);
			}

		}else {
			fprintf(stderr, "invalid C instruction[%s]\n", codeline);
			exit(0);
		}
	}else {
		fprintf(stderr, "invalid C instruction[%s]\n", codeline);
		exit(0);
	}

}

