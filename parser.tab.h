/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 557 "parser.y"

    typedef struct AstNode AstNode;

#line 53 "parser.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    AMPERSAND = 258,               /* AMPERSAND  */
    RPAREN_CastExpression = 259,   /* RPAREN_CastExpression  */
    TYPE_IDENTIFIER_IdentifierforLambdaParameterList = 260, /* TYPE_IDENTIFIER_IdentifierforLambdaParameterList  */
    VAR_IdentifierComplement_IdentifierforLambdaParameterList = 261, /* VAR_IdentifierComplement_IdentifierforLambdaParameterList  */
    YIELD_IdentifierComplement_IdentifierforLambdaParameterList = 262, /* YIELD_IdentifierComplement_IdentifierforLambdaParameterList  */
    TYPE_IDENTIFIER_MethodDeclarator = 263, /* TYPE_IDENTIFIER_MethodDeclarator  */
    VAR_MethodDeclarator = 264,    /* VAR_MethodDeclarator  */
    YIELD_MethodDeclarator = 265,  /* YIELD_MethodDeclarator  */
    TYPE_IDENTIFIER_AnnotationIdentifiers = 266, /* TYPE_IDENTIFIER_AnnotationIdentifiers  */
    VAR_AnnotationIdentifiers = 267, /* VAR_AnnotationIdentifiers  */
    YIELD_AnnotationIdentifiers = 268, /* YIELD_AnnotationIdentifiers  */
    TYPE_IDENTIFIER = 269,         /* TYPE_IDENTIFIER  */
    TYPE_IDENTIFIER_EnumDeclaration = 270, /* TYPE_IDENTIFIER_EnumDeclaration  */
    LANGLE = 271,                  /* LANGLE  */
    AT_Dims = 272,                 /* AT_Dims  */
    AT_Modifier = 273,             /* AT_Modifier  */
    AT_AnnotationTypeDeclaration = 274, /* AT_AnnotationTypeDeclaration  */
    LBRACK = 275,                  /* LBRACK  */
    LBRACK_ArrayAccess = 276,      /* LBRACK_ArrayAccess  */
    DEFAULT_SwitchLabel = 277,     /* DEFAULT_SwitchLabel  */
    DOT = 278,                     /* DOT  */
    DOT_CommonName = 279,          /* DOT_CommonName  */
    VAR_IdentifierComplement = 280, /* VAR_IdentifierComplement  */
    YIELD_IdentifierComplement = 281, /* YIELD_IdentifierComplement  */
    CLASS = 282,                   /* CLASS  */
    BYTE = 283,                    /* BYTE  */
    INT = 284,                     /* INT  */
    SHORT = 285,                   /* SHORT  */
    LONG = 286,                    /* LONG  */
    CHAR = 287,                    /* CHAR  */
    FLOAT = 288,                   /* FLOAT  */
    DOUBLE = 289,                  /* DOUBLE  */
    EXTENDS = 290,                 /* EXTENDS  */
    SUPER = 291,                   /* SUPER  */
    OPEN = 292,                    /* OPEN  */
    OPENS = 293,                   /* OPENS  */
    REQUIRES = 294,                /* REQUIRES  */
    EXPORTS = 295,                 /* EXPORTS  */
    USES = 296,                    /* USES  */
    PROVIDES = 297,                /* PROVIDES  */
    PACKAGE = 298,                 /* PACKAGE  */
    IMPORT = 299,                  /* IMPORT  */
    PUBLIC = 300,                  /* PUBLIC  */
    PROTECTED = 301,               /* PROTECTED  */
    PRIVATE = 302,                 /* PRIVATE  */
    ABSTRACT = 303,                /* ABSTRACT  */
    STATIC = 304,                  /* STATIC  */
    FINAL = 305,                   /* FINAL  */
    STRICTFP = 306,                /* STRICTFP  */
    TRANSITIVE = 307,              /* TRANSITIVE  */
    IMPLEMENTS = 308,              /* IMPLEMENTS  */
    TRANSIENT = 309,               /* TRANSIENT  */
    VOLATILE = 310,                /* VOLATILE  */
    SYNCHRONIZED = 311,            /* SYNCHRONIZED  */
    NATIVE = 312,                  /* NATIVE  */
    BOOLEAN = 313,                 /* BOOLEAN  */
    VOID = 314,                    /* VOID  */
    THROW = 315,                   /* THROW  */
    THROWS = 316,                  /* THROWS  */
    THIS = 317,                    /* THIS  */
    ENUM = 318,                    /* ENUM  */
    INTERFACE = 319,               /* INTERFACE  */
    DEFAULT = 320,                 /* DEFAULT  */
    VAR = 321,                     /* VAR  */
    IF = 322,                      /* IF  */
    ELSE = 323,                    /* ELSE  */
    ASSERT = 324,                  /* ASSERT  */
    SWITCH = 325,                  /* SWITCH  */
    CASE = 326,                    /* CASE  */
    DO = 327,                      /* DO  */
    WHILE = 328,                   /* WHILE  */
    FOR = 329,                     /* FOR  */
    BREAK = 330,                   /* BREAK  */
    YIELD = 331,                   /* YIELD  */
    CONTINUE = 332,                /* CONTINUE  */
    RETURN = 333,                  /* RETURN  */
    TRY = 334,                     /* TRY  */
    CATCH = 335,                   /* CATCH  */
    FINALLY = 336,                 /* FINALLY  */
    NEW = 337,                     /* NEW  */
    MODULE = 338,                  /* MODULE  */
    UnqualifiedMethodIdentifier = 339, /* UnqualifiedMethodIdentifier  */
    NUMBER = 340,                  /* NUMBER  */
    TRUE = 341,                    /* TRUE  */
    FALSE = 342,                   /* FALSE  */
    INTEGERLITERAL = 343,          /* INTEGERLITERAL  */
    FLOATINGPOINTLITERAL = 344,    /* FLOATINGPOINTLITERAL  */
    CHARACTERLITERAL = 345,        /* CHARACTERLITERAL  */
    STRINGLITERAL = 346,           /* STRINGLITERAL  */
    MY_NULL = 347,                 /* MY_NULL  */
    LE = 348,                      /* LE  */
    GE = 349,                      /* GE  */
    EQ = 350,                      /* EQ  */
    NE = 351,                      /* NE  */
    AND = 352,                     /* AND  */
    OR = 353,                      /* OR  */
    TO = 354,                      /* TO  */
    WITH = 355,                    /* WITH  */
    ARROW = 356,                   /* ARROW  */
    DIAMOND = 357,                 /* DIAMOND  */
    DOUBLE_COLON = 358,            /* DOUBLE_COLON  */
    MUL_ASSIGN = 359,              /* MUL_ASSIGN  */
    DIV_ASSIGN = 360,              /* DIV_ASSIGN  */
    MOD_ASSIGN = 361,              /* MOD_ASSIGN  */
    ADD_ASSIGN = 362,              /* ADD_ASSIGN  */
    SUB_ASSIGN = 363,              /* SUB_ASSIGN  */
    SHL_ASSIGN = 364,              /* SHL_ASSIGN  */
    SHR_ASSIGN = 365,              /* SHR_ASSIGN  */
    USHR_ASSIGN = 366,             /* USHR_ASSIGN  */
    AND_ASSIGN = 367,              /* AND_ASSIGN  */
    XOR_ASSIGN = 368,              /* XOR_ASSIGN  */
    OR_ASSIGN = 369,               /* OR_ASSIGN  */
    INSTANCEOF = 370,              /* INSTANCEOF  */
    SHL_OP = 371,                  /* SHL_OP  */
    SHR_OP = 372,                  /* SHR_OP  */
    USHR_OP = 373,                 /* USHR_OP  */
    INC_OP = 374,                  /* INC_OP  */
    DEC_OP = 375,                  /* DEC_OP  */
    PREFIX_INC = 376,              /* PREFIX_INC  */
    PREFIX_DEC = 377,              /* PREFIX_DEC  */
    ELLIPSIS = 378,                /* ELLIPSIS  */
    TRAILING_COMMA = 379,          /* TRAILING_COMMA  */
    TextBlock = 380,               /* TextBlock  */
    SEALED = 381,                  /* SEALED  */
    NON_SEALED = 382,              /* NON_SEALED  */
    SEALED_IdentifierComplement = 383, /* SEALED_IdentifierComplement  */
    NON_SEALED_IdentifierComplement = 384, /* NON_SEALED_IdentifierComplement  */
    PERMITS = 385,                 /* PERMITS  */
    EMPTY_STMT = 386,              /* EMPTY_STMT  */
    PREC_ConditionalExpression_1 = 387, /* PREC_ConditionalExpression_1  */
    PREC_ConditionalExpression = 388, /* PREC_ConditionalExpression  */
    PREC_ConditionalOrExpression_1 = 389, /* PREC_ConditionalOrExpression_1  */
    PREC_ConditionalAndExpression = 390, /* PREC_ConditionalAndExpression  */
    PREC_ConditionalAndExpression_1 = 391, /* PREC_ConditionalAndExpression_1  */
    PREC_InclusiveOrExpression = 392, /* PREC_InclusiveOrExpression  */
    PREC_InclusiveOrExpression_1 = 393, /* PREC_InclusiveOrExpression_1  */
    PREC_ExclusiveOrExpression = 394, /* PREC_ExclusiveOrExpression  */
    PREC_ExclusiveOrExpression_1 = 395, /* PREC_ExclusiveOrExpression_1  */
    PREC_AndExpression = 396,      /* PREC_AndExpression  */
    PREC_AndExpression_1 = 397,    /* PREC_AndExpression_1  */
    PREC_EqualityExpression = 398, /* PREC_EqualityExpression  */
    PREC_RelationalExpression = 399, /* PREC_RelationalExpression  */
    PREC_RelationalExpression_1 = 400, /* PREC_RelationalExpression_1  */
    PREC_SimpleRelationalExpression = 401, /* PREC_SimpleRelationalExpression  */
    PREC_ShiftExpression = 402,    /* PREC_ShiftExpression  */
    PREC_ShiftExpression_1 = 403,  /* PREC_ShiftExpression_1  */
    PREC_AdditiveExpression = 404, /* PREC_AdditiveExpression  */
    PREC_AdditiveExpression_1 = 405, /* PREC_AdditiveExpression_1  */
    PREC_MultiplicativeExpression = 406, /* PREC_MultiplicativeExpression  */
    LT_RelationalExpression = 407, /* LT_RelationalExpression  */
    RBRACK = 408                   /* RBRACK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 560 "parser.y"

    char* str;
    int val;
    AstNode* node;

#line 229 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */
