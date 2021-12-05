#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int Expression();   // prototyping

int check_if_exist(FILE *file, char* name)
{
    if (file == NULL)
    {
        printf("'%s' not found.\n", name);
        return 0;
    }
    else
    {
        printf("'%s' opened.\n", name);
        return 1;
    }
}

typedef struct{
    int type;       // type of token (there are 9)
    char str[100];  // value of token
    int length;     // length of token in number of bytes
}Token;

int ptr = 0;
int input_length = 0;
char* input_string;
enum type{TYPE_INTEGER, TYPE_FLOAT, TYPE_STRING, TYPE_CHAR, TYPE_COMMENT,
    TYPE_TYPE, TYPE_RESERVED, TYPE_OPERATOR, TYPE_IDENTIFIER, TYPE_INVALID, TYPE_WHITESPACE,
    TYPE_UNARY};
FILE *input_file, *printout;

struct Symbol{
    char name[100];         //num = 1
    char type[100];         //2
    int scope;              //3
    char* funcorvar;    //4
    int value;
};

#define MAX_SIZE 100
struct Symbol symbol_table[MAX_SIZE];
int count = 0;      // used for bracket
int symcount = 0;
int localdec = 1;
int sptr = 0;
int globalflag = 0;
int foundmain = 0;


const char* getType(enum type t){
    switch (t)
    {
        case TYPE_INTEGER:
            return "TYPE_INTEGER";
        case TYPE_FLOAT:
            return "TYPE_FLOAT";
        case TYPE_STRING:
            return "TYPE_STRING";
        case TYPE_CHAR:
            return "TYPE_CHAR";
        case TYPE_COMMENT:
            return "TYPE_COMMENT";
        case TYPE_TYPE:
            return "TYPE_TYPE";
        case TYPE_RESERVED:
            return "TYPE_RESERVED";
        case TYPE_OPERATOR: return "TYPE_OPERATOR";
        case TYPE_IDENTIFIER: return "TYPE_IDENTIFIER";
        case TYPE_INVALID: return "TYPE_INVALID";
        case TYPE_WHITESPACE: return "TYPE_WHITESPACE";
        case TYPE_UNARY: return "TYPE_UNARY";
    }
}

// advances nexttoken
void next(){
    ptr++;
    while(input_string[ptr] == ' ' || input_string[ptr] == '\t' || input_string[ptr] == '\n')
        ptr++;
}

// reached end of input
int doneWithInput(){
    //moves ptr past white spaces
    return 0;
}

int matchdatatype(char* id){    // should we have a struct type here?
    if (strcmp(id, "void") == 0 || strcmp(id, "char") == 0 || strcmp(id, "int") == 0 || strcmp(id, "float") == 0){
        return 1;
    }
    return 0;
}

int matchreserved(char id[]){
    if (strcmp(id, "sizeof") == 0 || strcmp(id, "enum") == 0 || strcmp(id, "case") == 0 || strcmp(id, "default") == 0 ||
        strcmp(id, "if") == 0 || strcmp(id, "else") == 0 || strcmp(id, "switch") == 0 || strcmp(id, "while") == 0 ||
        strcmp(id, "do") == 0 || strcmp(id, "for") == 0 || strcmp(id, "goto") == 0 || strcmp(id, "continue") == 0 ||
        strcmp(id, "break") == 0 || strcmp(id, "return") == 0){
        return 1;
    }
    return 0;
}

int matchoperator(char op){
    if (op == '+'  || op == '-'|| op == '*' || op == '/' || op == '<' || op == '>' ||
        op == '='  || op == '!'|| op == '&' || op == '?' || op == '[' || op == ']' ||
        op == '{' || op == '}' || op == '|' || op == '(' || op == ')' || op == '%' ||
        op == '^' || op == ',' || op == ':' || op == ';' || op == '#' || op == '~' ||
        op == '\\'){
        return 1;
    }

    return 0;
}

int foundwhitespace(char space){
    if (space == ' ' || space == '\t' || space == '\n')
        return 1;
    else
        return 0;
}

void printcolor(Token *token){

    if (strcmp(getType(token->type), "TYPE_TYPE") == 0){     //blue
        printf("\x1b[34m%s\x1b[0m", token->str);
    } else if (strcmp(getType(token->type), "TYPE_STRING") == 0 || strcmp(getType(token->type), "TYPE_CHAR") == 0){       //green
        printf("\x1b[32m%s\x1b[0m", token->str);
    } else if (strcmp(getType(token->type), "TYPE_INTEGER") == 0 || strcmp(getType(token->type), "TYPE_FLOAT") == 0){     //cyan
        printf("\x1b[36m%s\x1b[0m", token->str);
    } else if (strcmp(getType(token->type), "TYPE_RESERVED") == 0){      //purple
        printf("\x1b[35m%s\x1b[0m", token->str);
    } else if (strcmp(getType(token->type), "TYPE_COMMENT") == 0){       // hite (shaded)
        printf("\x1b[37m%s\x1b[0m", token->str);
    } else if (strcmp(getType(token->type), "TYPE_OPERATOR") == 0){      //yellow
        printf("\x1b[33m%s\x1b[0m", token->str);
    } else if (strcmp(getType(token->type), "TYPE_INVALID") == 0){       //bright red
        printf("\x1b[91m%s\x1b[0m", token->str);
    }
    else{   //regular
        printf("%s", token->str);
    }
}

Token accept(Token *token, int tptr, enum type type1, char* line){
    int len = tptr - ptr +1;
    token->type = type1;
    token->length = len;

    for (int i = 0; i < len; i++){
        token->str[i] = line[ptr +i];
    }
    token->str[len] = '\0';

    // if it's an identifier - check to reserved and type types
    if(matchreserved(token->str) == 1)
        token->type = TYPE_RESERVED;
    if (matchdatatype(token->str) == 1)
        token->type = TYPE_TYPE;

    ptr = tptr;


//    return token;
}

// fill empty token with next token
void identifyNextToken(Token *token){

    int tptr = ptr;
    int state = 0;

    // pass through white spaces
    if(input_string[ptr] == ' ' || input_string[ptr] == '\n' || input_string[ptr] == '\t'){
        token->str[0] = input_string[ptr];
        token->str[1] = '\0';
        token->type = TYPE_WHITESPACE;
//        return token;
    }
        // check for operators, comments, includes and defines
    else if (matchoperator(input_string[ptr]) == 1){
        if (input_string[ptr] == '/' && input_string[tptr+1] == '/'){
            while (input_string[tptr] != '\n'){
                tptr++;
            }
            accept(token,tptr, TYPE_COMMENT, input_string);
        }
        else if (input_string[tptr] == '/' && input_string[tptr+1] == '*') {
            tptr += 3;
            while (input_string[tptr] != '*' && input_string[tptr + 1] != '/') {
                tptr++;
            }
            tptr +=1; // capture the second operator for closing comment
            accept(token, tptr, TYPE_COMMENT, input_string);
        }
        else if (input_string[ptr] == '#' && input_string[ptr+1] == 'i') {    //handle library inclusion (state 2)
            int count = 0;
            char word[10];

            while(input_string[tptr] != ' '){
                tptr++;
                word[count] = input_string[tptr];
                count++;
            }
            word[count-1] = '\0';
            while(input_string[tptr] != '\n'){
                tptr++;
            }

            if(strcmp(word, "include") == 0 || strcmp(word, "define") == 0)    // handle include libraries - type invalid for now
                accept(token, tptr, TYPE_COMMENT, input_string);
            else {
                accept(token, tptr, TYPE_INVALID, input_string);
                printf("1___%d\n", tptr);
            }
        }
        else {

            if (input_string[tptr] == '+' && (input_string[tptr +1] == '+' || input_string[tptr +1] == '=')){
                tptr++;
                accept(token, tptr, TYPE_OPERATOR, input_string);
            }
            else if (input_string[tptr] == '-' && (input_string[tptr +1] == '-' || input_string[tptr +1] == '=')){
                tptr++;
                accept(token, tptr, TYPE_OPERATOR, input_string);
            }
            else if ((input_string[tptr] == '=' || input_string[tptr] == '<' || input_string[tptr] == '>' || input_string[tptr] == '!')
                     && input_string[tptr +1] == '='){
                tptr++;
                accept(token, tptr, TYPE_OPERATOR, input_string);
            }
            else if (input_string[tptr] == '&' && input_string[tptr +1] == '&'){
                tptr++;
                accept(token, tptr, TYPE_OPERATOR, input_string);
            }
            else if (input_string[tptr] == '|' && input_string[tptr +1] == '|'){
                tptr++;
                accept(token, tptr, TYPE_OPERATOR, input_string);
            }
            else
                accept(token, tptr, TYPE_OPERATOR, input_string);
        }

    }
        // check for char
    else if (input_string[ptr] == '\''){
        // make sure there is appropriate spacing for char
        if (input_string[ptr+1] == '\\')    // handle backslash chars
            tptr = ptr+3;
        else
            tptr = ptr + 2;  // move to end of char (closing ')

        if (input_string[tptr] == '\'')
            accept(token, tptr, TYPE_CHAR, input_string);
        else {  // if there is no closing apostrophe, syntax error
            {      //syntax error, char has too many inputs
                while (input_string[tptr] != '\'')
                {
                    tptr++;
                    if (input_string[tptr] == '\0')
                        break;
                }
                accept(token, tptr, TYPE_INVALID, input_string);
            }
        }
    }
        // check for string
    else if (input_string[ptr] == '"'){
        tptr++;
        while (input_string[tptr] != '"'){
            tptr++;
            if (input_string[tptr] == '\0') // no ending quotes, syntax error
                break;
        }
        if (input_string[ptr] == input_string[tptr])
            accept(token, tptr, TYPE_STRING, input_string);
        else {
            accept(token, tptr, TYPE_INVALID, input_string);
            printf("2___%d\n", tptr);
        }
    }
        // check for integers   -- need to check for hexidecimals
    else if (input_string[ptr] >= '0' && input_string[ptr] <= '9'){
        state = 1;
        while (input_string[tptr] >= '0' && input_string[tptr] <= '9'){
            tptr++;
            // check for float (state 2)
            if (input_string[tptr] == '.'){
                tptr++; //move past decimal
                while (input_string[tptr] >= '0' && input_string[tptr] <= '9') {
                    tptr++;
                }
                state = 2;
            }
        }
        tptr--; // decrement due to while loop
        if (state == 2)
            accept(token, tptr, TYPE_FLOAT, input_string);
        else
            accept(token, tptr, TYPE_INTEGER, input_string);
    }
        //check for identifiers (state 1)
    else if ((input_string[ptr] >= 65 && input_string[ptr] <= 90) || (input_string[ptr] >= 97 && input_string[ptr] <= 122)){            // start with a letter
        while ((input_string[tptr] >= 65 && input_string[tptr] <= 90) || (input_string[tptr] >= 97 && input_string[tptr] <= 122) ||     // if its a letter
               (input_string[tptr] >= 48 && input_string[tptr] <= 57) || input_string[tptr] == '_' || input_string[tptr] == '.'){       // or digits, _, or .
            tptr++;
        }
        tptr--; // decrement due to while loop
        // make sure no operator following is being added to string
        if (input_string[tptr] == '=' || input_string[tptr] == '(' || input_string[tptr] == '[' || input_string[tptr] == ' ' ||
            matchoperator(input_string[tptr]) == 1){
            tptr--;
        }
        accept(token, tptr, TYPE_IDENTIFIER, input_string);
    }
    else if (input_string[ptr] == '\0') {
        return;
    }
    else{
        accept(token, tptr, TYPE_INVALID, input_string);
        printf("3___token: %s   [%d]\n", token->str, tptr);
//        exit(0);
    }

}

int match(char* string, int type){
    int saveptr = ptr;
    Token t;
    identifyNextToken(&t);
    ptr = saveptr;
    if (strcmp(t.str, string) == 0 && t.type == type)
        return 1;

    return 0;
}

// find the type of the next token
int matchtype(int type){

    int saveptr = ptr;
    Token t;
    identifyNextToken(&t);
    ptr = saveptr;
    if (t.type == type)
        return 1;
    return 0;
}

int PrimaryExpression(){

    // get a token that is not a whitespace or comment
    int saveptr = ptr;
    Token t;
    identifyNextToken(&t);
//    ptr = saveptr;

    printf("got primary expression %s: %s    [%d]\n", getType(t.type), t.str, ptr);

    if (strcmp(getType(t.type), "TYPE_INTEGER") == 0){
        // got integer so move ptr;
        next();
        return atoi(t.str);
    }
    else if (strcmp(getType(t.type), "TYPE_FLOAT") == 0){
        // floats will be rounded to the nearest whole digit and turned into an int
        int i = 0;
        while (t.str[i] != '.'){
            i++;
        }
        int val;
        if (t.str[i+1] >= '5')
            val = atoi(t.str) +1;
        else
            val = atoi(t.str);

        next();
        return val;
    }
    else if (strcmp(getType(t.type), "TYPE_STRING") == 0){
        next();
        return 0;
    }
    else if (strcmp(getType(t.type), "TYPE_CHAR") == 0){
        next();
        // return decimal value here
        if (t.str[1] == '\\')
            return t.str[2];
        else
            return t.str[1];
    }
    else if (strcmp(getType(t.type), "TYPE_IDENTIFIER") == 0){
        if(getsymbol(t.str) >= 0){
            printf("    pulling identifier value '%s = %d' for expression\n",
                   symbol_table[getsymbol(t.str)].name, symbol_table[getsymbol(t.str)].value);
            next();
            globalflag = 0;
            return symbol_table[getsymbol(t.str)].value;
        }
        else {
            next();
            return 0;
        }
    }
    else if (strcmp(t.str, "(") == 0){
        next();
        int val = Expression();
        return val;
    }
    return -99;
}

int PostFixExpression(){

    int firstvalue = PrimaryExpression();

    return firstvalue;
}

int UnaryExpression(){

    int saveptr = ptr;
    Token prevtoken;
    identifyNextToken(&prevtoken);
    ptr = saveptr;

    int firstvalue = PostFixExpression();

    if (matchtype(TYPE_OPERATOR) && (strcmp(getType(prevtoken.type), "TYPE_OPERATOR") == 0 || ptr == 0)){
        Token utoken;
        identifyNextToken(&utoken);

//        printf("(u) %s  %s\n", prevtoken.str, utoken.str);

        if (match("-", TYPE_OPERATOR)) {
            printf("    unary token: %s\n", utoken.str);
            next();
            int nextvalue = PostFixExpression();
            return -nextvalue;
        }
//        else if (match("+", TYPE_OPERATOR)){                  //
//            printf("    unary token: %s\n", utoken.str);
//            next();
//            int nextvalue = PostFixExpression();
//            return nextvalue;
//        }
        else if (match("!", TYPE_OPERATOR)){
            next();
            int nextvalue = UnaryExpression();  // need to call unary for - token to check for negative numbers
            if (nextvalue == 0)
                return 1;
            else
                return 0;
        }
    }

    if (firstvalue == -99){
        Token t;
        identifyNextToken(&t);
        next();
        printf("__either expression sums to -99 or you got an incorrect entry for primary expression__\n"
               "error token: %s\n"
               "note: if expresssion is a +number with no spaces, this error will also come up so space it out\n\n", t.str);
    }

    return firstvalue;

}

int MultiplicativeExpression(){

    int firstvalue = UnaryExpression();


    while(1){

        int saveptr = ptr;
        Token optoken;
        identifyNextToken(&optoken);
        ptr = saveptr;

        if (match("*", TYPE_OPERATOR) || match("/", TYPE_OPERATOR) || match("%", TYPE_OPERATOR)){
            printf("    multi token: %s       [%d]\n", optoken.str, ptr);

            // move ptr to next token
            next();
            int nextvalue = UnaryExpression();

            if (strcmp(optoken.str, "*") == 0){
                firstvalue *= nextvalue;
            }
            else if (strcmp(optoken.str, "/") == 0){
                firstvalue /= nextvalue;
            }
            else
                firstvalue %= nextvalue;

        }
        else break;
    }

    return firstvalue;
}

int AdditiveExpression(){

    int firstvalue = MultiplicativeExpression();


    while(1){

        int saveptr = ptr;
        Token optoken;
        identifyNextToken(&optoken);
        ptr = saveptr;

        if (match("+", TYPE_OPERATOR) || match("-", TYPE_OPERATOR)){
            printf("    add token: %s       [%d]\n", optoken.str, ptr);

            // move ptr to next token and get its value
            next();
            int nextvalue = MultiplicativeExpression();


            if (strcmp(optoken.str, "+") == 0){
                firstvalue += nextvalue;
            }
            else
                firstvalue -= nextvalue;

        }
        else break;
    }

    return firstvalue;

}

int RelationalExpression(){

    int firstvalue = AdditiveExpression();

    while(1){

        if (input_string[ptr] == '"' || (input_string[ptr] == '\'') || (input_string[ptr] == '.'))    // hard fix for type string or char, stack smash error
            next();

        Token reltoken;
        identifyNextToken(&reltoken);

        if (strcmp(reltoken.str, "<") == 0 || strcmp(reltoken.str, "<=") == 0 || strcmp(reltoken.str, "==") == 0
            || strcmp(reltoken.str, "!=") == 0 || strcmp(reltoken.str, ">") == 0 || strcmp(reltoken.str, ">=") == 0){
            printf("    relational operator: %s     [%d]\n", reltoken.str, ptr);
            next();
            int nextvalue = AdditiveExpression();

            if (strcmp(reltoken.str, "<") == 0)
                firstvalue = firstvalue < nextvalue;
            else if (strcmp(reltoken.str, "<=") == 0) {
                firstvalue = firstvalue <= nextvalue;
            }
            else if (strcmp(reltoken.str, ">") == 0)
                firstvalue = firstvalue > nextvalue;
            else if (strcmp(reltoken.str, ">=") == 0)
                firstvalue = firstvalue >= nextvalue;
            else if (strcmp(reltoken.str, "!=") == 0)
                firstvalue = firstvalue != nextvalue;
            else
                firstvalue = firstvalue == nextvalue;

        }
        else break;
    }

    return firstvalue;
}

int LogicalExpression(){

    int firstvalue = RelationalExpression();

    while(1){

        if (match(")", TYPE_OPERATOR)) {     // move past ) when we get it
            next();
            printf("    found closing ')' token\n");
        }

        if (input_string[ptr] == '"' || (input_string[ptr] == '\'') || (input_string[ptr] == '.'))    // hard fix for type string or char, stack smash error
            next();

        ptr--;
        Token logtoken;
        identifyNextToken(&logtoken);
        ptr++;


        if (strcmp(logtoken.str, "&&") == 0 || strcmp(logtoken.str, "||") == 0){
            printf("    logical operator: %s     [%d]\n", logtoken.str, ptr);
            next();
            int nextvalue = RelationalExpression();
//            printf("%d  %d\n",firstvalue, nextvalue);

            if (strcmp(logtoken.str, "&&") == 0)
                if (firstvalue == 1 && nextvalue == 1)
                    return 1;
                else
                    return 0;
            else {
                if (firstvalue == 1 || nextvalue == 1)
                    return 1;
                else
                    return 0;
            }

        }
        else break;
    }
    return firstvalue;
}


int Expression(){

    int firstvalue = LogicalExpression();
    printf("\tanswer is: %d     [%d]\n", firstvalue, ptr);

    return firstvalue;
}

void printtoken(){
    if (ptr > input_length){
        printf("cannot print token since we are now out of bounds\n");
    }
    else {
        int saveptr = ptr;
        Token t;
        identifyNextToken(&t);
        printf("*__current token is: %s    [%d-%d]  %d\n", t.str, saveptr, ptr, input_length);
        ptr = saveptr;
    }
}

int matchnexttoken(char* str){
    next();
    int saveptr = ptr;
    Token t;
    identifyNextToken(&t);
    ptr = saveptr;

    if (strcmp(t.str, str) == 0){
        return 1;
    }
    return 0;
}

void tonexttoken(){
    Token t;
    identifyNextToken(&t);
    next();
}

void addsymbol(Token t, int num){

    if (num == 1) {
        for (int i = 0; i < t.length; i++) {
            symbol_table[symcount].name[i] = t.str[i];
        }
    }
    else if (num == 2) {
        for (int i = 0; i < t.length; i++) {
            symbol_table[symcount].type[i] = t.str[i];
        }
    }
    else if (num == 3) {
        // do something for scope
    }
//    else if (num == 4) {
//        for (int i = 0; i < t.length; i++) {
//            symbol_table[symcount].funcorvar[i] = t.str[i];
//        }
//    }
}

int getsymbol(char* id){

    for(int i = 0; i < symcount; i++){
        if(strcmp(symbol_table[i].name, id) == 0){
            return i;
        }
    }
    return -1;
}

int DecSpecifier(){

    sptr = ptr;
    Token t;
    identifyNextToken(&t);

//    printf("%s\n", t.str);
    if (strcmp(getType(t.type), "TYPE_TYPE") == 0) {
        printf("got specifier: %s   [%d-%d]\n", t.str, sptr, ptr);
        addsymbol(t, 2);
        printf("    got symbol type: %s  [%d]\n", symbol_table[symcount].type, symcount);

//        next();

        // see if we have a ptr * char after the type
        int saveptr = ptr;
        next();
        Token star;
        identifyNextToken(&star);

        if (strcmp(star.str, "*") == 0) {
            printf("\tgot * char pointer\n");
            strcat(symbol_table[symcount].type, star.str);
            printf("\tgot symbol %s\n", symbol_table[symcount].type);
        }
        else
            ptr = saveptr;

        return 1;
    }
    return 0;
}

int Identifier(){
    sptr = ptr;
    Token t;
    identifyNextToken(&t);

    if (strcmp(getType(t.type), "TYPE_IDENTIFIER") == 0) {
        printf("got identifier: %s   [%d-%d]\n", t.str, sptr, ptr);
        addsymbol(t, 1);
        printf("    updated symbol name to: %s  [%d]\n", symbol_table[symcount].name, symcount);
//        next();

        return 1;
    }
    // ; and ) ends up here maybe } too
    return 0;
}

int ParameterList(){
    Token t;

    while (strcmp(t.str, ")") != 0) {
        identifyNextToken(&t);
        next();
    }
    printf("    (finished parameter list for function %s\n", t.str);    // next() will leave ptr before {
}

int ifStatement(){                  // did not handle single action statement if/else statements without {}
    printf("    if statement\n");
    next();
    int flag = Expression();   // token will be at ( to express condition and be at {
    tonexttoken();  // move passed {
    ptr--;

    if(flag){
        printf("    if condition is true\n");

        while(!matchnexttoken("}")){
            Statement();
        }
        if (match("}", TYPE_OPERATOR)) {
            printf("    if statement complete\n");  // move passed }
            next();
        }
        if(match("else", TYPE_RESERVED)){
            printf("    passing through else statement\n");
//            printf("%d\n", ptr);
            while(input_string[ptr] != '}')
                next();
//            printf("%d\n", ptr);
        }

    }
    else{
        printf("    if condition is false, flag value: %d\n", flag);    // currently, at opening { for if
//        printf("%d\n", ptr);
        while(input_string[ptr] != '}') // pass through if statement
            next();
//        printf("%d\n", ptr);

        tonexttoken();      // moved passed closing } from if statement

        if (match("else", TYPE_RESERVED)){
            printf("    entering else statement\n");
            tonexttoken();  // brings you to opening {
            ptr--;
            next();
            while(!matchnexttoken("}")){
                Statement();
            }
            ptr--;
        }
    }


}

int whileStatement(){
    printf("    while statement\n");
    next(); // move passed (
    int flagptr = ptr;  // save ptr to after opening (
    int flag = Expression();   // token will be at ( to express condition and be at {
    int startptr = ptr;    // location is the opening bracket to while loop
    tonexttoken();  // move passed {
    ptr--;

    while (!matchnexttoken("}")){   // complete what's inside of while
        // moves ptr through while loop
    }
    int endptr = ptr;
    printf("    got ptrs for while block %d %d; flag ptr: %d\n", startptr, endptr, flagptr);
    ptr = startptr;

    // if flag is false - skip while loop
    if(!flag){
        ptr = endptr;
    }
    else {
        while (flag) {

            while (!matchnexttoken("}")) {   // complete what's inside of while
                Statement();
            }
            next();
            ptr = flagptr;
            flag = Expression();

            if (flag) {
                ptr = startptr;
            } else {
                ptr = endptr;
                break;
            }
        }
    }

}

int Statement(){
    printf("entering statement\n");

    if(match("if", TYPE_RESERVED)) {
        tonexttoken();
        ifStatement();
        ptr--;
        tonexttoken();
        if(match("}", TYPE_OPERATOR)){
            next();
        }
        ptr--;
        printf("exited if statement\n");

    }
    else if (match("while", TYPE_RESERVED)){
        tonexttoken();
        whileStatement();
    }
    else if(matchtype(TYPE_IDENTIFIER)){
        printf("handle statement identifier\n");

        Token id;       // get token id to update symbol later
        identifyNextToken(&id);

        if (getsymbol(id.str) >= 0){
            printf("    updating symbol '%s' value through expression\n", id.str);
            next();
            if(match("=", TYPE_OPERATOR)){
                next(); // moves passed =
                globalflag = 1;
                symbol_table[getsymbol(id.str)].value = Expression();
            }

        }

    }
    else{   // this will also handle expression statements, might need to change this to if token is {
        printf("statement called compound statement\n");
        CompoundStatement();

    }
}

int CompoundStatement(){        // declaration or statement
    sptr=ptr;

    if (matchtype(TYPE_TYPE)){ // if type type, declaration
        Declaration();
        if(match("=", TYPE_OPERATOR)) {
            next();
            printf("    updating symbol '%s' value through expression\n", symbol_table[symcount-1].name);
            symbol_table[symcount-1].value = Expression();  // decrement symcount because Declaration will
        }                                                   // already increment before getting its value
    }
    // do regular expressions
    else if (matchtype(TYPE_INTEGER) || matchtype(TYPE_FLOAT))
    {
        printf("got an expression (no specifier)\n");
        Expression();
    }
    else{
        printf("compound statement calls: ");
        Statement();
    }


}

int Function(){
    printf("got a function\n");
    ParameterList();

    while (!matchnexttoken("}") && ptr < input_length){
        CompoundStatement();
    }

}


int Declaration(){
    sptr = ptr;
    printf("got a Declaration\n");
    symbol_table[symcount].funcorvar = "variable";
    if (localdec){
        printf("local variable declaration\n");
        DecSpecifier();
        next();  // move passed specifier
        Identifier();
        next();
    }
    else {  // add expression to global variable
        symbol_table[symcount].value = Expression();
        next(); // move passed declaration closer ;
    }
    symcount++;

}

int Program(){

    while(ptr < input_length -1){
        DecSpecifier();
        next();  // move passed specifier
        Identifier();
        next();

        Token t;
        identifyNextToken(&t);
        if(strcmp(t.str, "(") == 0){
            printf("got function '%s' declaration\n", symbol_table[symcount].name);
            symbol_table[symcount].funcorvar = "function";

            while(strcmp(symbol_table[symcount].name, "main") != 0){    // move passed by all non-main function
                while (input_string[ptr] != '}' && ptr < input_length-1){                       // and still update symbol table
                    ptr++;
                }
                if (ptr > input_length-2) {
                    printf("    end of program\n");
                    break;
                }
                printtoken();
                next();     // move passed closing } from function block
                symcount++;
                DecSpecifier();
                next();
                Identifier();
                next();
                symbol_table[symcount].funcorvar = "function";

            }// end of function while
            symcount++;
            if (ptr > input_length-2) {
                printf("    end of program\n");
                break;
            }
            next(); // moved passed opening ( before parameterlist call within function method
            Function();
//            printtoken();     // this should be } for closing functions
            next();

        }
        else if (strcmp(t.str, "=") == 0 || strcmp(t.str, ";") == 0)
        {
            printf("    this is a global var declaration\n");
            next();
            localdec = 0;
            int saveptr = ptr;      // this may cause problems later
            if (strcmp(t.str, ";") != 0) {
                Declaration();
            }
            else
            {
                symbol_table[symcount].funcorvar = "variable";
                symcount++;
            }
            ptr = saveptr;
            localdec = 1;
        }
    }// end of program while

}

int FindBracket(){

    Token t;
    int bptr = ptr;
    int aptr = ptr;

    while (ptr < input_length) {
        if (matchtype(TYPE_OPERATOR)) {
            identifyNextToken(&t);
            if (strcmp(t.str, "{") == 0) {

                bptr = ptr;
                next();
                count++;

                FindBracket();
//                aptr = ptr;
//                printf("1\t[%d] %c\n", bptr, input_string[bptr]);
                printf("%d\n", bptr);
                printf("bracket at %d points to ", bptr);
            }
            else if (strcmp(t.str, "}") == 0){

                bptr = ptr;
//                printf("2\t[%d] %c\n", bptr, input_string[bptr]);
                printf("%d\n", bptr);
                printf("bracket at %d points to ", bptr);
                break;
            }
        }

        next();

    }


    return count;
}


int main(int argc, char* argv[]) {

    // run provided file
    char* input = "test.txt";
    input_file = fopen(input, "r");

    if (argv[1] != '\0')    // if something is passed through terminal
        input = argv[1];

    input_file = fopen(input, "r");
    if (check_if_exist(input_file, input) == 0)         // if nothing can open, close program
        exit(1);

    char read;
    read = fgetc(input_file);
    // get length of file for string building
    while (read != EOF) {
        input_length++;
        read = fgetc(input_file);
    }
    printf("\n\n");

    // put the file into a string
    input_string=(char*)malloc(input_length);
    rewind(input_file);
    for(int i=0; i<input_length; i++){
        input_string[i]=fgetc(input_file);
    }
    input_string[input_length] = 0;
    fclose(input_file);
//    printf("%d  %c\n", input_length, input_string[input_length]);


//    Expression();
//    FindBracket();
    Program();

//    printf("\n");
//    int to[] = {80, 87, 68, 90, 96, 101, 29, 103, 184, 188};
//    int at[] = {84, 80, 87, 68, 99, 96, 101, 29, 186, 184};
//    for (int i = 0; i < 10; i++) {
//        printf("%c  %c\n", input_string[to[i]], input_string[at[i]]);
//    }

    printf("Symbol Table (%d)\n", symcount);
    for (int i = 0; i < symcount; i++){
        printf("    %s %s %s %d\n", symbol_table[i].funcorvar, symbol_table[i].type, symbol_table[i].name, symbol_table[i].value);
    }
//    printf("__\n");
//    for (int i = 0; i < symcount; i++){
//        printf("    %s %s %s\n", symfov, symt, symn);
//    }
    printf("%d\n", input_length);

    return 0;
}