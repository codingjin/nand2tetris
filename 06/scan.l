%{
#include <stdio.h>
extern int yylex();
%}
%option noyywrap
%%
"//".*	;
[ \r\t]	;
%%

int main()
{
	while (yylex());
	return 0;
}

