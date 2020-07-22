/*
    This file is a part of Tiny-Shading-Language or TSL, an open-source cross
    platform programming shading language.

    Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.

    TSL is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

%{
/*
    --------------------------------------------------------------------
    WARNING:
            This file is automatically generated, do not modify.
    --------------------------------------------------------------------
*/
	#include <string>
	#include "tsl_version.h"
	#include "compiler/ast.h"
	#include "compiler/types.h"
	#include "compiler/compiler.h"
    #include "compiler/str_helper.h"
    #include "system/impl.h"

	USE_TSL_NAMESPACE

	#define scanner tsl_compiler->get_scanner()

	int yylex( union YYSTYPE *,struct YYLTYPE *,void * );
    void yyerror(struct YYLTYPE* loc, void *tsl_compiler, char const *str);
	int g_verbose = 0;	// somehow bool is not working here.
%}

/* definitions of tokens and types passed by FLEX */
%union {
    class AstNode 					*p;	/* pointers for the AST struct nodes */
	float							f;  /* floating point value cache. */
    double                          d;  /* double precision floating point value cache. */
	int								i;  /* integer value or enum values. */
	const char						*s;	/* string values. */
	char							c;  /* single char. */
	Tsl_Namespace::DataType			t;	/* data type. */
	Tsl_Namespace::VariableConfig	vc; /* 'out', 'in' */
}

%locations
%define api.pure
%lex-param {void * scanner}
%parse-param {class Tsl_Namespace::TslCompiler * tsl_compiler}

%token <s> ID
%token <i> INT_NUM
%token <f> FLT_NUM
%token <d> DBL_NUM
%token INC_OP			"++"
%token DEC_OP			"--"
%token <vc> OUT			"out"
%token <vc> IN			"in"
%token <vc> CONST		"const"
%token SHADER_FUNC_ID
%token <i> TYPE_INT	    "int"
%token <i> TYPE_FLOAT	"float"
%token <i> TYPE_DOUBLE  "double"
%token <i> TYPE_BOOL	"bool"
%token <i> TYPE_COLOR	"color"
%token <i> TYPE_VECTOR	"vector"
%token TYPE_VOID		"void"
%token EOL              ";"
%token L_CBRACKET       "{"
%token R_CBRACKET       "}"
%token L_RBRACKET       "("
%token R_RBRACKET       ")"
%token L_SBRACKET       "["
%token R_SBRACKET       "]"
%token OP_ADD           "+"
%token OP_MINUS         "-"
%token OP_MULT          "*"
%token OP_DIV           "/"
%token OP_MOD			"%"
%token OP_AND			"&"
%token OP_OR			"|"
%token OP_XOR			"^"
%token OP_LOGIC_AND     "&&"
%token OP_LOGIC_OR		"||"
%token OP_EQ			"=="
%token OP_NE			"!="
%token OP_GE			">="
%token OP_G				">"
%token OP_LE			"<="
%token OP_L				"<"
%token OP_SHL			"<<"
%token OP_SHR			">>"
%token OP_ADD_ASSIGN    "+="
%token OP_MINUS_ASSIGN  "-="
%token OP_MULT_ASSIGN   "*="
%token OP_DIV_ASSIGN    "/="
%token OP_MOD_ASSIGN    "%="
%token OP_ASSIGN        "="
%token OP_AND_ASSIGN	"&="
%token OP_OR_ASSIGN		"|="
%token OP_XOR_ASSIGN	"^="
%token OP_SHL_ASSIGN	"<<="
%token OP_SHR_ASSIGN	">>="
%token OP_NOT			"!"
%token OP_COMP			"~"
%token DOT				"."
%token COMMA            ","
%token COLON            ":"
%token METADATA_START   "<<<"
%token METADATA_END     ">>>"
%token RETURN		    "return"
%token QUESTION_MARK	"?"
%token IF				"if"
%token ELSE				"else"
%token FOR				"for"
%token WHILE			"while"
%token DO				"do"
%token TRUE             "true"
%token FALSE            "false"
%token BREAK            "break"
%token CONTINUE         "continue"
%token CLOSURE          "closure"
%token MAKE_CLOSURE     "make_closure"
%token GLOBAL_VALUE     "global_value"
%token STRUCT			"struct"
%token TEXTURE2D_HANDLE "texture"
%token TEXTURE2D_SAMPLE "texture2d_sample"
%token TEXTURE2D_SAMPLE_ALPHA "texture2d_sample_alpha"
%token SHADER_RESOURCE_HANDLE

%type <p> PROGRAM FUNCTION_ARGUMENT_DECL FUNCTION_ARGUMENT_DECLS SHADER_FUNCTION_ARGUMENT_DECLS FUNCTION_BODY VARIABLE_LVALUE ID_OR_FIELD FUNCTION_ARGUMENTS SHADER_FUNCTION_ARGUMENT_DECL FOR_INIT_STATEMENT STRUCT_DEF
%type <p> STATEMENT STATEMENTS STATEMENT_RETURN STATEMENT_EXPRESSION_OPT STATEMENT_EXPRESSION STATEMENT_VARIABLES_DECLARATION STATEMENT_CONDITIONAL STATEMENT_LOOP 
%type <p> STATEMENT_SCOPED STATEMENT_LOOPMOD STATEMENT_STRUCT_MEMBERS_DECLARATION
%type <p> EXPRESSION_CONST EXPRESSION_BINARY EXPRESSION EXPRESSION_VARIABLE EXPRESSION_FUNCTION_CALL EXPRESSION_TERNARY EXPRESSION_OPT EXPRESSION_SCOPED EXPRESSION_ASSIGN EXPRESSION_UNARY 
%type <p> EXPRESSION_TYPECAST EXPRESSION_MAKE_CLOSURE EXPRESSION_TEXTURE_SAMPLE EXPRESSION_FLOAT3_CONSTRUCTOR
%type <c> OP_UNARY
%type <t> TYPE
%type <i> REC_OR_DEC
%type <vc> ARGUMENT_CONFIG ARGUMENT_CONFIGS_OPT ARGUMENT_CONFIGS

%nonassoc IF_THEN
%nonassoc ELSE

%left ","
%right "=" "+=" "-=" "*=" "/=" "%=" "<<=" ">>=" "&=" "|=" "^="
%right "?" ":"
%left "||"
%left "&&"
%left "|"
%left "^"
%left "&"
%left "==" "!="
%left ">" ">=" "<" "<=" 
%left "<<" ">>"
%left "+" "-"
%left "*" "/" "%"
%right UMINUS_PREC "!" "~"
%left "++" "--"
%left "(" ")"
%left "[" "]"
%left "<<<" ">>>"

/* the start token */
%start PROGRAM

%%
// A programm has a bunch of global statement.
PROGRAM:
	// empty shader
	{
        $$ = nullptr;
	}
	|
	GLOBAL_STATEMENTS {
	};

// One or multiple of blobal statements
GLOBAL_STATEMENTS:
	GLOBAL_STATEMENT{
	}
	|
	GLOBAL_STATEMENT GLOBAL_STATEMENTS {
	};

// Global statement could be one of the followings
//  - Global function definition.
//  - Global data structure definition.
//  - Shader function definition.
//  - Texture declaration.
GLOBAL_STATEMENT:
    SHADER_DEF {
    }
	|
	FUNCTION_DEF {
	}
	|
	STRUCT_DEF {
	}
    |
    STATEMENT_TEXTURE_DEF {
    }
    |
    STATEMENT_SHADER_RESOURCE_HANDLE_DEF {
    }
    |
    GLOBAL_STATEMENT_VARIABLES_DECLARATION
    {
    };

GLOBAL_STATEMENT_VARIABLES_DECLARATION:
	TYPE ID ";"
	{
        
	}
    |
    TYPE ID "[" EXPRESSION "]" ";"
    {
        
    }
    |
    TYPE ID "=" EXPRESSION ";"
    {
		
    }
    |
    TYPE ID "[" EXPRESSION "]" "=" ARRAY_INITIALIZER ";"
    {
    };

STATEMENT_SHADER_RESOURCE_HANDLE_DEF:
    SHADER_RESOURCE_HANDLE ID ";"{
        AstNode_Statement* texture_declare = new AstNode_Statement_ShaderResourceHandleDeclaration( $2 );
        tsl_compiler->push_global_parameter(texture_declare);
    };

STATEMENT_TEXTURE_DEF:
    "texture" ID ";"{
         AstNode_Statement* texture_declare = new AstNode_Statement_TextureDeclaration( $2 );
         tsl_compiler->push_global_parameter(texture_declare);
    };

STRUCT_DEF:
	"struct" ID "{" STATEMENT_STRUCT_MEMBERS_DECLARATION "}" ";"
	{
		AstNode_Statement_StructMemberDecls* members = AstNode::castType<AstNode_Statement_StructMemberDecls>($4);
		AstNode_StructDeclaration* struct_declaration = new AstNode_StructDeclaration( $2 , members );
		tsl_compiler->push_structure_declaration(struct_declaration);
		$$ = struct_declaration;
	};

STATEMENT_STRUCT_MEMBERS_DECLARATION:
	STATEMENT_VARIABLES_DECLARATION
    {
        AstNode_Statement_StructMemberDecls* node = new AstNode_Statement_StructMemberDecls();
        AstNode_Statement_VariableDecl* member = AstNode::castType<AstNode_Statement_VariableDecl>($1);
        node->add_member_decl(member);

        $$ = node;
    }
	|
	STATEMENT_STRUCT_MEMBERS_DECLARATION STATEMENT_VARIABLES_DECLARATION
	{
        AstNode_Statement_StructMemberDecls* members = AstNode::castType<AstNode_Statement_StructMemberDecls>($1);
        AstNode_Statement_VariableDecl* member = AstNode::castType<AstNode_Statement_VariableDecl>($2);
        $$ = members->add_member_decl(member);
	}
	;

// Shader is the only unit that can be exposed in the group.
SHADER_DEF:
	SHADER_FUNC_ID ID "(" SHADER_FUNCTION_ARGUMENT_DECLS ")" FUNCTION_BODY {
		AstNode_FunctionBody*		body = AstNode::castType<AstNode_FunctionBody>($6);
		AstNode_MultiVariableDecl*	variables = AstNode::castType<AstNode_MultiVariableDecl>($4);
        const char*                 name = tsl_compiler->get_shader_root_function_name().c_str();
		AstNode_FunctionPrototype*	function = new AstNode_FunctionPrototype(name, variables, body, true);
		tsl_compiler->push_function(function, true);
	};

SHADER_FUNCTION_ARGUMENT_DECLS:
	/* empty */
	{
		$$ = nullptr;
	}
	|
	SHADER_FUNCTION_ARGUMENT_DECL
    {
        AstNode_MultiVariableDecl* node = new AstNode_MultiVariableDecl();
        AstNode_SingleVariableDecl* var_decl = AstNode::castType<AstNode_SingleVariableDecl>($1);
        $$ = node->add_var(var_decl);
	}
	|
	SHADER_FUNCTION_ARGUMENT_DECLS "," SHADER_FUNCTION_ARGUMENT_DECL {
		AstNode_MultiVariableDecl* node = AstNode::castType<AstNode_MultiVariableDecl>($1);
        AstNode_SingleVariableDecl* var_decl = AstNode::castType<AstNode_SingleVariableDecl>($3);
        $$ = node->add_var(var_decl);
	};

SHADER_FUNCTION_ARGUMENT_DECL:
	FUNCTION_ARGUMENT_DECL ARGUMENT_METADATA {
	};

ARGUMENT_METADATA:
	// no meta data
	{}
	|
	"<<<" ">>>"{
	};

// Standard function definition
FUNCTION_DEF:
	TYPE ID "(" FUNCTION_ARGUMENT_DECLS ")" FUNCTION_BODY {
		AstNode_FunctionBody*		body = AstNode::castType<AstNode_FunctionBody>($6);
		AstNode_MultiVariableDecl*	arguments = AstNode::castType<AstNode_MultiVariableDecl>($4);
		AstNode_FunctionPrototype*	function = new AstNode_FunctionPrototype($2, arguments, body, false, $1);

        tsl_compiler->push_function(function);
	};

FUNCTION_ARGUMENT_DECLS:
	/* empty */
	{
		$$ = nullptr;
	}
	|
	FUNCTION_ARGUMENT_DECL
	{
        AstNode_MultiVariableDecl* node = new AstNode_MultiVariableDecl();
        AstNode_SingleVariableDecl* var_decl = AstNode::castType<AstNode_SingleVariableDecl>($1);
        $$ = node->add_var(var_decl);
	}
	|
	FUNCTION_ARGUMENT_DECLS "," FUNCTION_ARGUMENT_DECL{
        AstNode_MultiVariableDecl* node = AstNode::castType<AstNode_MultiVariableDecl>($1);
        AstNode_SingleVariableDecl* var_decl = AstNode::castType<AstNode_SingleVariableDecl>($3);
        $$ = node->add_var(var_decl);
	};

FUNCTION_ARGUMENT_DECL:
	ARGUMENT_CONFIGS_OPT TYPE ID {
		VariableConfig config = $1;
		AstNode_SingleVariableDecl* node = new AstNode_SingleVariableDecl($3, $2, config);
		$$ = node;
	}
	|
	ARGUMENT_CONFIGS_OPT TYPE ID "=" EXPRESSION {
		VariableConfig config = $1;
		AstNode_Expression* init_exp = AstNode::castType<AstNode_Expression>($5);
		AstNode_SingleVariableDecl* node = new AstNode_SingleVariableDecl($3, $2, config, init_exp);
		$$ = node;
	};

ARGUMENT_CONFIGS_OPT:
	/* empty */
	{
		$$ = VariableConfig::NONE;
	}
	|
	ARGUMENT_CONFIGS
	{
		$$ = $1;
	};

ARGUMENT_CONFIGS:
	ARGUMENT_CONFIG
	{
		$$ = $1;
	}
	|
	ARGUMENT_CONFIG ARGUMENT_CONFIGS
	{
		int config0 = $1;
		int config1 = $2;
		$$ = VariableConfig( config0 | config1 );
	};

ARGUMENT_CONFIG:
	"in"
	{
		$$ = VariableConfig::INPUT;
	}
	|
	"out"
	{
		$$ = VariableConfig::OUTPUT;
	}
	|
	"const"
	{
		$$ = VariableConfig::CONST;
	};

FUNCTION_BODY:
	"{" STATEMENTS "}" {
		AstNode_Statement*	statements = AstNode::castType<AstNode_Statement>($2);
		$$ = new AstNode_FunctionBody(statements);
	}
    |
    ";"
    {
        $$ = nullptr;
    };

STATEMENTS:
	STATEMENTS STATEMENT{
		AstNode_Statement* statements = AstNode::castType<AstNode_Statement>$1;
		AstNode_Statement* statement = AstNode::castType<AstNode_Statement>$2;
		if(!statements)
			$$ = statement;
		else if(!statement)
			$$ = statements;
        else{
            if( auto compound_statement = AstNode::castType<AstNode_CompoundStatements>(statements, false) ){
                compound_statement->append_statement(statement);
                $$ = statements;
            }else{
                AstNode_CompoundStatements* ret = new AstNode_CompoundStatements();
                ret->append_statement(statements);
                ret->append_statement(statement);
                $$ = ret;
            }
        }
	}
	|
	/* empty */ {
		$$ = nullptr;
	};

STATEMENT:
	STATEMENT_SCOPED
	|
	STATEMENT_RETURN
    |
    STATEMENT_LOOPMOD
	|
	STATEMENT_VARIABLES_DECLARATION
	|
	STATEMENT_CONDITIONAL
	|
	STATEMENT_LOOP
	|
	STATEMENT_EXPRESSION;

STATEMENT_LOOPMOD:
    "break" ";"
    {
        $$ = new AstNode_Statement_Break ();
    }
    | 
    "continue" ";"
    {
        $$ = new AstNode_Statement_Continue ();
    }
    ;
    
STATEMENT_SCOPED:
	"{" STATEMENTS "}"
	{
        AstNode_Statement* statement = AstNode::castType<AstNode_Statement>($2);
        $$ = new AstNode_ScoppedStatement(statement);
	}
	;

STATEMENT_RETURN:
	"return" STATEMENT_EXPRESSION_OPT ";"
	{
		AstNode_Expression* expression = AstNode::castType<AstNode_Expression>($2);
		$$ = new AstNode_Statement_Return(expression);
	};

STATEMENT_EXPRESSION_OPT:
	EXPRESSION {
	}
	|
	/* empty */ {
		$$ = nullptr;
	};

STATEMENT_VARIABLES_DECLARATION:
	TYPE ID ";"
	{
        const DataType type = $1;
		AstNode_SingleVariableDecl* var = new AstNode_SingleVariableDecl($2, type);
		$$ = new AstNode_Statement_VariableDecl(var);
	}
    |
    TYPE ID "=" EXPRESSION ";"
    {
		const DataType type = $1;
		AstNode_Expression* init_exp = AstNode::castType<AstNode_Expression>($4);
		AstNode_SingleVariableDecl* var = new AstNode_SingleVariableDecl($2, type, VariableConfig::NONE, init_exp);
		
        $$ = new AstNode_Statement_VariableDecl(var);
    }
    |
    TYPE ID "[" EXPRESSION "]" ";"
    {
        const DataType type = $1;
        AstNode_Expression* cnt = AstNode::castType<AstNode_Expression>($4);
		AstNode_ArrayDecl* var = new AstNode_ArrayDecl($2, type, cnt);

        $$ = new AstNode_Statement_VariableDecl(var);
    }
    |
    TYPE ID "[" EXPRESSION "]" "=" ARRAY_INITIALIZER ";"
    {
        const DataType type = $1;
        AstNode_Expression* cnt = AstNode::castType<AstNode_Expression>($4);
		AstNode_ArrayDecl* var = new AstNode_ArrayDecl($2, type, cnt);

        $$ = new AstNode_Statement_VariableDecl(var);
    };

ARRAY_INITIALIZER:
    "{" ARRAY_DATA_OPT "}"
    {
    };

ARRAY_DATA_OPT:
    {
    }
    |
    EXPRESSION_CONST
    {
    }
    |
    EXPRESSION_CONST "," ARRAY_DATA_OPT
    {
    };

STATEMENT_CONDITIONAL:
	"if" "(" EXPRESSION ")" STATEMENT %prec IF_THEN {
		AstNode_Expression* cond = AstNode::castType<AstNode_Expression>($3);
		AstNode_Statement*	true_statements = AstNode::castType<AstNode_Statement>($5);
		$$ = new AstNode_Statement_Condition( cond , true_statements );
	}
	|
	"if" "(" EXPRESSION ")" STATEMENT "else" STATEMENT {
		AstNode_Expression* cond = AstNode::castType<AstNode_Expression>($3);
		AstNode_Statement*	true_statements = AstNode::castType<AstNode_Statement>($5);
		AstNode_Statement*	false_statements = AstNode::castType<AstNode_Statement>($7);
		$$ = new AstNode_Statement_Condition( cond, true_statements, false_statements );
	};
	
STATEMENT_LOOP:
	"while" "(" EXPRESSION ")" STATEMENT{
		AstNode_Expression* cond = AstNode::castType<AstNode_Expression>($3);
		AstNode_Statement*	statements = AstNode::castType<AstNode_Statement>($5);
		$$ = new AstNode_Statement_Loop_While( cond , statements );
	}
	|
	"do" STATEMENT "while" "(" EXPRESSION ")" ";"{
		AstNode_Expression* cond = AstNode::castType<AstNode_Expression>($5);
		AstNode_Statement*	statements = AstNode::castType<AstNode_Statement>($2);
		$$ = new AstNode_Statement_Loop_DoWhile( cond , statements );
	}
	|
	"for" "(" FOR_INIT_STATEMENT EXPRESSION_OPT ";" EXPRESSION_OPT ")" STATEMENT {
        AstNode_Statement*  init = AstNode::castType<AstNode_Statement>($3);
        AstNode_Expression* cond = AstNode::castType<AstNode_Expression>($4);
        AstNode_Expression* iter = AstNode::castType<AstNode_Expression>($6);
		AstNode_Statement*	statements = AstNode::castType<AstNode_Statement>($8);
		$$ = new AstNode_Statement_Loop_For(init, cond, iter, statements);
	};
	
FOR_INIT_STATEMENT:
	";"{
        $$ = nullptr;
	}
	|
	STATEMENT_EXPRESSION{
	}
	|
	STATEMENT_VARIABLES_DECLARATION {
	};
	
STATEMENT_EXPRESSION:
	EXPRESSION ";" {
		AstNode_Expression* expression = AstNode::castType<AstNode_Expression>($1);
		$$ = new AstNode_Statement_Expression(expression);
	}

EXPRESSION_OPT:
	/* empty */ 
	{
		$$ = nullptr;
	}
	|
	EXPRESSION;

// Exrpession always carries a value so that it can be used as input for anything needs a value,
// like if condition, function parameter, etc.
EXPRESSION:
	EXPRESSION_UNARY
	|
	EXPRESSION_BINARY
	|
	EXPRESSION_TERNARY
	|
	EXPRESSION_ASSIGN
	|
	EXPRESSION_FUNCTION_CALL
	|
	EXPRESSION_CONST
	|
	EXPRESSION_SCOPED
	|
	EXPRESSION_TYPECAST
	|
	EXPRESSION_VARIABLE
    |
    EXPRESSION_MAKE_CLOSURE
    |
    EXPRESSION_TEXTURE_SAMPLE
    |
    EXPRESSION_FLOAT3_CONSTRUCTOR
    ;

EXPRESSION_FLOAT3_CONSTRUCTOR:
    TYPE_COLOR "(" FUNCTION_ARGUMENTS ")"{
		AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($3);
		$$ = new AstNode_Float3Constructor( args );
    }
    |
    TYPE_VECTOR "(" FUNCTION_ARGUMENTS ")"{
		AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($3);
		$$ = new AstNode_Float3Constructor( args );
    };
    
EXPRESSION_TEXTURE_SAMPLE:
    "texture2d_sample" "<" ID ">" "(" FUNCTION_ARGUMENTS ")"{
        AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($6);
		$$ = new AstNode_Expression_Texture2DSample( $3 , args );
    }
    |
    "texture2d_sample_alpha" "<" ID ">" "(" FUNCTION_ARGUMENTS ")"{
        AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($6);
		$$ = new AstNode_Expression_Texture2DSample( $3 , args , true );
    }

EXPRESSION_UNARY:
	OP_UNARY EXPRESSION %prec UMINUS_PREC {
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($2);
		switch( $1 ){
			case '+':
				$$ = new AstNode_Unary_Pos(exp);	// it is still necessary to wrap something here to prevent this value from being a lvalue later.
				break;
			case '-':
				$$ = new AstNode_Unary_Neg(exp);
				break;
			case '!':
				$$ = new AstNode_Unary_Not(exp);
				break;
			case '~':
				$$ = new AstNode_Unary_Compl(exp);
				break;
			default:
				$$ = nullptr;
		}
	};
	
OP_UNARY:
	"-" {
		$$ = '-';
	}
	|
	"+" {
		$$ = '+';
	}
	|
	"!" {
		$$ = '!';
	}
	|
	"~" {
		$$ = '~';
	};

EXPRESSION_BINARY:
	EXPRESSION "&&" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_And( left , right );
	}
	|
	EXPRESSION "||" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Or( left , right );
	}
	|
	EXPRESSION "&" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Bit_And( left , right );
	}
	|
	EXPRESSION "|" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Bit_Or( left , right );
	}
	|
	EXPRESSION "^" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Bit_Xor( left , right );
	}
	|
	EXPRESSION "==" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Eq( left , right );
	}
	|
	EXPRESSION "!=" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Ne( left , right );
	}
	|
	EXPRESSION ">" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_G( left , right );
	}
	|
	EXPRESSION "<" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_L( left , right );
	}
	|
	EXPRESSION ">=" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Ge( left , right );
	}
	|
	EXPRESSION "<=" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Le( left , right );
	}
	|
	EXPRESSION "<<" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Shl( left , right );
	}
	|
	EXPRESSION ">>" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Shr( left , right );
	}
	|
	EXPRESSION "+" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Add( left , right );
	}
	|
	EXPRESSION "-" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Minus( left , right );
	}
	|
	EXPRESSION "*" EXPRESSION {
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Multi( left , right );
	}
	|
	EXPRESSION "/" EXPRESSION{
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Div( left , right );
	}
	|
	EXPRESSION "%" EXPRESSION{
		AstNode_Expression* left = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* right = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_Binary_Mod( left , right );
	};

// Ternary operation support
EXPRESSION_TERNARY:
	EXPRESSION "?" EXPRESSION ":" EXPRESSION {
		AstNode_Expression* cond = AstNode::castType<AstNode_Expression>($1);
		AstNode_Expression* true_expr = AstNode::castType<AstNode_Expression>($3);
		AstNode_Expression* false_expr = AstNode::castType<AstNode_Expression>($5);
		$$ = new AstNode_Ternary( cond , true_expr , false_expr );
	};

// Assign an expression to a reference
EXPRESSION_ASSIGN:
	VARIABLE_LVALUE "=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_Eq( var , exp );
	}
	|
	VARIABLE_LVALUE "+=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_AddEq( var , exp );
	}
	|
	VARIABLE_LVALUE "-=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_MinusEq( var , exp );
	}
	|
	VARIABLE_LVALUE "*=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_MultiEq( var , exp );
	}
	|
	VARIABLE_LVALUE "/=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_DivEq( var , exp );
	}
	|
	VARIABLE_LVALUE "%=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_ModEq( var , exp );
	}
	|
	VARIABLE_LVALUE "&=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_AndEq( var , exp );
	}
	|
	VARIABLE_LVALUE "|=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_OrEq( var , exp );
	}
	|
	VARIABLE_LVALUE "^=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_XorEq( var , exp );
	}
	|
	VARIABLE_LVALUE "<<=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_ShlEq( var , exp );
	}
	|
	VARIABLE_LVALUE ">>=" EXPRESSION {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($3);
		$$ = new AstNode_ExpAssign_ShrEq( var , exp );
	};

// Function call, this is only non-shader function. TSL doesn't allow calling shader function.
EXPRESSION_FUNCTION_CALL:
	ID "(" FUNCTION_ARGUMENTS ")" {
		AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($3);
		$$ = new AstNode_FunctionCall( $1 , args );
	};

// a special function just for creating closure data structure.
EXPRESSION_MAKE_CLOSURE:
    "make_closure" "<" ID ">" "(" FUNCTION_ARGUMENTS ")"
    {
        AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($6);
		$$ = new AstNode_Expression_MakeClosure( $3 , args );

        // notify the compiler to generate pre-decleration
        tsl_compiler->closure_touched( $3 );
    };

// None-shader function arguments
FUNCTION_ARGUMENTS:
	{
		$$ = nullptr;
	}
	|
	EXPRESSION
    {
        AstNode_Expression* arg = AstNode::castType<AstNode_Expression>($1);
        AstNode_ArgumentList* args = new AstNode_ArgumentList();
        $$ = args->add_argument(arg);
    }
	|
	FUNCTION_ARGUMENTS "," EXPRESSION {
        AstNode_ArgumentList* args = AstNode::castType<AstNode_ArgumentList>($1);
        AstNode_Expression* arg = AstNode::castType<AstNode_Expression>($3);
		$$ = args->add_argument( arg );
	};


// Const literal
EXPRESSION_CONST:
	INT_NUM {
		$$ = new AstNode_Literal_Int( $1 );
	}
	|
	FLT_NUM {
		$$ = new AstNode_Literal_Flt( $1 );
	}
    |
    DBL_NUM {
        $$ = new AstNode_Literal_Double( $1 );
    }
    |
    "true" {
        $$ = new AstNode_Literal_Bool( true );
    }
    |
    "false" {
        $$ = new AstNode_Literal_Bool( false );
    }
    |
    "global_value" "<" ID ">"{
        $$ = new AstNode_Literal_GlobalValue( $3 );
    };

// Scopped expression
EXPRESSION_SCOPED:
	"(" EXPRESSION ")" {
		$$ = $2;
	};

// This is for type casting
EXPRESSION_TYPECAST:
	"(" TYPE ")" EXPRESSION {
		const DataType type = $2;
		AstNode_Expression* exp = AstNode::castType<AstNode_Expression>($4);
		$$ = new AstNode_TypeCast(exp, type);
	};

EXPRESSION_VARIABLE:
	VARIABLE_LVALUE{
		$$ = $1;
	}
	|
	VARIABLE_LVALUE REC_OR_DEC {
		AstNode_Lvalue* exp = AstNode::castType<AstNode_Lvalue>($1);
		if( $2 == 1 )
			$$ = new AstNode_Expression_PostInc(exp);
		else if( $2 == 2 )
			$$ = new AstNode_Expression_PostDec(exp);
		else{
			// this should not happen
			$$ = exp;
		}
	}
	|
	REC_OR_DEC VARIABLE_LVALUE {
		AstNode_Lvalue* exp = AstNode::castType<AstNode_Lvalue>($2);
		if( $1 == 1 )
			$$ = new AstNode_Expression_PreInc(exp);
		else if( $1 == 2 )
			$$ = new AstNode_Expression_PreDec(exp);
		else{
			// this should not happen
			$$ = exp;
		}
	};

REC_OR_DEC:
	"++" {
		$$ = 1;
	}
	|
	"--" {
		$$ = 2;
	};

// No up to two dimensional array supported for now.
VARIABLE_LVALUE:
	ID_OR_FIELD;

ID_OR_FIELD:
	ID{
		$$ = new AstNode_VariableRef($1);
	}
    |
    VARIABLE_LVALUE "[" EXPRESSION "]"{
        AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
        AstNode_Expression* index = AstNode::castType<AstNode_Expression>($3);
        $$ = new AstNode_ArrayAccess(var, index);
    }
	|
	VARIABLE_LVALUE "." ID {
		AstNode_Lvalue* var = AstNode::castType<AstNode_Lvalue>($1);
		$$ = new AstNode_StructMemberRef(var, $3);
	};

TYPE:
	"int" {
		$$ = { DataTypeEnum::INT , nullptr };
		tsl_compiler->cache_next_data_type($$);
	}
	|
	"float" {
		$$ = { DataTypeEnum::FLOAT , nullptr };
		tsl_compiler->cache_next_data_type($$);
	}
    |
    "double" {
        $$ = { DataTypeEnum::DOUBLE , nullptr };
        tsl_compiler->cache_next_data_type($$);
    }
	|
	"vector" {
		const char* s = make_str_unique("float3");
		DataType type = { DataTypeEnum::STRUCT , s };
		$$ = type;

		tsl_compiler->cache_next_data_type(type);
	}
	|
	"color" {
		const char* s = make_str_unique("float3");
		DataType type = { DataTypeEnum::STRUCT , s };
		$$ = type;

		tsl_compiler->cache_next_data_type(type);
	}
	|
	"bool" {
		$$ = { DataTypeEnum::BOOL , nullptr };
		tsl_compiler->cache_next_data_type($$);
	}
	|
	"void" {
		$$ = { DataTypeEnum::VOID , nullptr };
		tsl_compiler->cache_next_data_type($$);
	}
    |
    "closure" {
        $$ = { DataTypeEnum::CLOSURE , nullptr };
		tsl_compiler->cache_next_data_type($$);
    }
	|
	"struct" ID {
		// This gramma is purely just to save me some time to implement the struct feature.
		// In an ideal world, it should just use the name of the struct, however it generates
		// a conflict, I don't have time to dig in for now.

		const char* s = make_str_unique($2);
		DataType type = { DataTypeEnum::STRUCT , s };
		$$ = type;

		tsl_compiler->cache_next_data_type(type);
	}
    ;
%%

void yyerror(struct YYLTYPE* loc, void* x, char const * str){
	if(!g_verbose)
		return;

	// line number is incorrect for now
	emit_error( "line(%d, %d), error: %s", loc->first_line, loc->first_column, str);
}

void makeVerbose(int verbose){
	 g_verbose = verbose;
}
