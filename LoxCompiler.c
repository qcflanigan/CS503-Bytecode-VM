    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "common.h"
    #include "LoxCompiler.h"
    #include "memory.h"
    #include "LoxScanner.h"

    #ifdef DEBUG_PRINT_CODE
    #include "LoxDebugger.h"
    #endif


    typedef struct {
        LoxToken current;
        LoxToken previous;
        bool hadError;
        bool resync;
    } LoxParser;

    typedef enum {
        PREC_NONE,
        PREC_ASSIGNMENT,  // =
        PREC_OR,          // or
        PREC_AND,         // and
        PREC_EQUALITY,    // == !=
        PREC_COMPARISON,  // < > <= >=
        PREC_TERM,        // + -
        PREC_FACTOR,      // * /
        PREC_UNARY,       // ! -
        PREC_CALL,        // . ()
        PREC_PRIMARY
    } LoxPrecedence;
  
    typedef void (*ParseFunc)(bool assignable);


    typedef struct {
        ParseFunc prefix;
        ParseFunc infix;
        LoxPrecedence precedence;
    } LoxParsePrecRule;


    typedef struct {
        LoxToken name;
        int depth;
        bool isCaptured;
    } Local;

    typedef struct {
        uint8_t index;
        bool isLocal;
    } Upvalue;
  
    typedef enum {
        TYPE_FUNCTION,
        TYPE_INITIALIZER,
        TYPE_METHOD,
        TYPE_SCRIPT
    } FunctionType;

    typedef struct Compiler {
        struct Compiler* enclosing;

        LoxObjFunction* function;
        FunctionType type;

        Local locals[UINT8_COUNT];
        int localCount;
        Upvalue upvalues[UINT8_COUNT];
        int scopeDepth;
    } LoxCompiler;


    typedef struct ClassCompiler {
        struct ClassCompiler* enclosing;
        bool hasSuperclass;
    } ClassCompiler;

    LoxParser parser;
  
    LoxCompiler* current = NULL;

    ClassCompiler* currentClass = NULL;

    static LoxChunk* currentChunk() {
        return &current->function->chunk;
    }
   
    static void errorAt(LoxToken* token, const char* msg) {
        if (parser.resync) return;

        parser.resync = true;
        fprintf(stderr, "[line %d] Error", token->line);

        if (token->type == TOKEN_EOF) {
            fprintf(stderr, " at end");
        } else if (token->type == TOKEN_ERROR) {
            // do nothing, already handling error
        } 
        else{
            fprintf(stderr, " at '%.*s'", token->length, token->start);
        }

        fprintf(stderr, ": %s\n", msg);
        parser.hadError = true;
    }

    static void parseError(const char* msg){
        errorAt(&parser.previous, msg);
    }
   
    static void parseErrorAtCurrent(const char* msg) {
        errorAt(&parser.current, msg);
    }
  
    static void advance() {
        parser.previous = parser.current;

        for (;;){
            parser.current = scanToken();
            if (parser.current.type != TOKEN_ERROR) break;

            parseErrorAtCurrent(parser.current.start);
        }
    }
  
    static void consumeToken(TokenType type, const char* message) {
        if (parser.current.type == type) {
            advance();
            return;
        }

        parseErrorAtCurrent(message);
    }

    static bool check(TokenType type) {
        return parser.current.type == type;
    }

    static bool match(TokenType type) {
        if (!check(type)) return false;
        advance();
        return true;
    }
   
    static void emitByte(uint8_t byte) {
        writeChunk(currentChunk(), byte, parser.previous.line);
    }

    static void emitBytes(uint8_t byte1, uint8_t byte2) {
        emitByte(byte1);
        emitByte(byte2);
    }
    
    static void emitLoop(int loopStart) {
        emitByte(OP_LOOP);

        int offset = currentChunk()->count - loopStart + 2;
        if (offset > UINT16_MAX) parseError("Loop body too large.");

        emitByte((offset >> 8) & 0xff);
        emitByte(offset & 0xff);
    }
    
    static int emitJump(uint8_t instruction) {
        emitByte(instruction);
        emitByte(0xff);
        emitByte(0xff);
        return (currentChunk()->count - 2);
    }
   
    static void emitReturn() {
        if (current->type == TYPE_INITIALIZER){
            emitBytes(OP_GET_LOCAL, 0);
        } 
        else{
            emitByte(OP_NIL);
        }

        emitByte(OP_RETURN);
    }
  
    static uint8_t makeConstant(LoxValue value){
        int constant = addConstant(currentChunk(), value);
        if (constant > UINT8_MAX){
            parseError("Too many constants in one chunk");
            return 0;
        }

        return (uint8_t)constant;
    }

    static void emitConstant(LoxValue value) {
        emitBytes(OP_CONSTANT, makeConstant(value));
    }

    static void patchJump(int offset) {
        int jump = currentChunk()->count - offset - 2;

        if (jump > UINT16_MAX) {
            parseError("Too much code to jump over");
        }

        currentChunk()->code[offset] = (jump >> 8) & 0xff;
        currentChunk()->code[offset + 1] = jump & 0xff;
    }
  
    //initialize the compiler for the lox code, sets scope values, environments, etc
    static void initCompiler(LoxCompiler* compiler, FunctionType type) {
        compiler->enclosing = current;
        compiler->function = NULL;
        compiler->type = type;
        compiler->localCount = 0;
        compiler->scopeDepth = 0;
        compiler->function = newFunction();
        current = compiler;
        if (type != TYPE_SCRIPT) {
            current->function->name = copyString(parser.previous.start, parser.previous.length);
        }

        Local* local = &current->locals[current->localCount++];
        local->depth = 0;
        local->isCaptured = false;
        if (type != TYPE_FUNCTION) {
            local->name.start = "this";
            local->name.length = 4;
        } 
        else {
            local->name.start = "";
            local->name.length = 0;
        }
}
  
    static LoxObjFunction* endCompiler() {
        emitReturn();
        LoxObjFunction* function = current->function;

        #ifdef DEBUG_PRINT_CODE
        if (!parser.hadError) {
            disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
    }
        #endif

        current = current->enclosing;
        return function;
    }

    //create new scope for any new classes, funcs, loops, etc
    static void beginScope() {
        current->scopeDepth++;
    }
   
    //close the scope of the current Lox environments - functions, classes, loops, etc
    static void endScope() {
        current->scopeDepth--;
        while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
            if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
            } 
            else {
                emitByte(OP_POP);
            }
            current->localCount--;
        }
    }

    //declare functions here to avoid any undeclared/unreferenced errors
    static void handleExpression();
    static void handleStatement();
    static void handleDeclaration();
    static LoxParsePrecRule* getRule(TokenType type);
    static void parserPrecedence(LoxPrecedence precedence);


    static uint8_t identifierConstant(LoxToken* name) {
        return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
    }
  
    static bool identifiersEqual(LoxToken* left, LoxToken* right) {
        if (left->length != right->length) return false;
        return memcmp(left->start, right->start, left->length) == 0;
    }
   
    static int resolveLocal(LoxCompiler* compiler, LoxToken* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
        if (local->depth == -1) {
            parseError("Can't read local variable in its own initializer");
        }
        return i;
        }
    }
    return -1;
}

    static int addUpvalue(LoxCompiler* compiler, uint8_t index, bool isLocal) {
        int upvalueCount = compiler->function->upvalueCount;

        for (int i = 0; i < upvalueCount; i++) {
            Upvalue* upvalue = &compiler->upvalues[i];
            if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
            }
        }


        if (upvalueCount == UINT8_COUNT) {
            parseError("Too many closure variables in function.");
            return 0;
        }

        compiler->upvalues[upvalueCount].isLocal = isLocal;
        compiler->upvalues[upvalueCount].index = index;
        return compiler->function->upvalueCount++;
}

    static int resolveUpvalue(LoxCompiler* compiler, LoxToken* name) {
        if (compiler->enclosing == NULL) return -1;

        int local = resolveLocal(compiler->enclosing, name);
        if (local != -1) {
            compiler->enclosing->locals[local].isCaptured = true;
            return addUpvalue(compiler, (uint8_t)local, true);
        }

        int upvalue = resolveUpvalue(compiler->enclosing, name);
        if (upvalue != -1) {
            return addUpvalue(compiler, (uint8_t)upvalue, false);
        }
        
        return -1;
    }

    static void addLocal(LoxToken name) {
        if (current->localCount == UINT8_COUNT) {
            parseError("Too many local variables in function.");
            return;
        }

        Local* local = &current->locals[current->localCount++];
        local->name = name;
        local->depth = -1;
        local->isCaptured = false;
    }

    //parse through the local variables of the current scope, 
    //add the variable if it is unique within a non-zero scope
    static void declareVariable() {
        if (current->scopeDepth == 0) return;

        LoxToken* name = &parser.previous;

        for (int i = current->localCount - 1; i >= 0; i--) {
            Local* local = &current->locals[i];
            if (local->depth != -1 && local->depth < current->scopeDepth) {
                break; 
            }

            //check for variables with same name already in the scope
            if (identifiersEqual(name, &local->name)) {
                parseError("Already a variable with this name in this scope");
            }
        }

        addLocal(*name);
    }
    
    static uint8_t parseVariable(const char* errorMessage) {
        consumeToken(TOKEN_IDENTIFIER, errorMessage);

        declareVariable();
        if (current->scopeDepth > 0) return 0;

        return identifierConstant(&parser.previous);
    }
 
    //mark the variable as initialized to avoid conflict of declaration
    static void markInitialized() {
        if (current->scopeDepth == 0) return;
        current->locals[current->localCount - 1].depth =
            current->scopeDepth;
    }


    static void defineVariable(uint8_t global) {
        if (current->scopeDepth > 0) {
            markInitialized();
            return;
        }

        emitBytes(OP_DEFINE_GLOBAL, global);
    }

    //checking the arguments passed to a func, process the list of arguments for each declared functions
    static uint8_t argumentList() {
        uint8_t argCount = 0;
        if (!check(TOKEN_RIGHT_PAREN)) {
            //while there are more arguments to process, use expression func to handle each one
            do {
                handleExpression();
                if (argCount == 255) {
                    parseError("Can't have more than 255 arguments");
            }
                argCount++;
            } while (match(TOKEN_COMMA));
        }
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
        return argCount;
    }
    
    static void and_(bool canAssign) {
        int endJump = emitJump(OP_JUMP_IF_FALSE);

        emitByte(OP_POP);
        parserPrecedence(PREC_AND);

        patchJump(endJump);
    }

    static void binaryOps(bool assignable) {
        TokenType operatorType = parser.previous.type;
        LoxParsePrecRule* rule = getRule(operatorType);
        parserPrecedence((LoxPrecedence)(rule->precedence + 1));

        //handle each type of binary operations
        switch (operatorType) {
            case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
            case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
            case TOKEN_GREATER:       emitByte(OP_GREATER); break;
            case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
            case TOKEN_LESS:          emitByte(OP_LESS); break;
            case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
            case TOKEN_PLUS:          emitByte(OP_ADD); break;
            case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
            case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
            case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
            default: 
                return;
        }
    }
  
    static void call(bool assignable) {
        uint8_t argCount = argumentList();
        emitBytes(OP_CALL, argCount);
    }

    static void dot(bool assignable) {
        consumeToken(TOKEN_IDENTIFIER, "Expect property name after '.'.");
        uint8_t name = identifierConstant(&parser.previous);

        if (assignable && match(TOKEN_EQUAL)) {
            handleExpression();
            emitBytes(OP_SET_PROPERTY, name);
        } 
        else if (match(TOKEN_LEFT_PAREN)) {
            uint8_t argCount = argumentList();
            emitBytes(OP_INVOKE, name);
            emitByte(argCount);
        } 
        else {
            emitBytes(OP_GET_PROPERTY, name);
        }
    }

    static void handleLiterals(bool assignable) {
        switch (parser.previous.type) {
            case TOKEN_FALSE: emitByte(OP_FALSE); break;
            case TOKEN_NIL: emitByte(OP_NIL); break;
            case TOKEN_TRUE: emitByte(OP_TRUE); break;
            default: 
                return; 
        }
    }

    static void handleGrouping(bool canAssign) {
        handleExpression();
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }

    static void handleNumbers(bool canAssign) {
        double value = strtod(parser.previous.start, NULL);

        emitConstant(NUMBER_VAL(value));
    }
   
    static void or_(bool canAssign) {
        int elseJump = emitJump(OP_JUMP_IF_FALSE);
        int endJump = emitJump(OP_JUMP);

        patchJump(elseJump);
        emitByte(OP_POP);

        parserPrecedence(PREC_OR);
        patchJump(endJump);
    }

    static void handleStr(bool canAssign) {
        emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
    }

    static void namedVariable(LoxToken name, bool assignable) {
        uint8_t getOp, setOp;
        int arg = resolveLocal(current, &name);
        if (arg != -1) {
            getOp = OP_GET_LOCAL;
            setOp = OP_SET_LOCAL;
        } 
        else if ((arg = resolveUpvalue(current, &name)) != -1) {
            getOp = OP_GET_UPVALUE;
            setOp = OP_SET_UPVALUE;
        } 
        else {
            arg = identifierConstant(&name);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }
        if (assignable && match(TOKEN_EQUAL)){
            handleExpression();
            emitBytes(setOp, (uint8_t)arg);
        } 
        else{
            emitBytes(getOp, (uint8_t)arg);
        }
    }

    static void handleVariables(bool assignable) {
        namedVariable(parser.previous, assignable);
    }

    static LoxToken syntheticToken(const char* text) {
        LoxToken token;
        token.start = text;
        token.length = (int)strlen(text);
        return token;
    }


    static void super_(bool canAssign) {
        if (currentClass == NULL) {
            parseError("Can't use 'super' outside of a class");
        } 
        else if (!currentClass->hasSuperclass) {
            parseError("Can't use 'super' in a class with no superclass");
        }

        consumeToken(TOKEN_DOT, "Expect '.' after 'super'");
        consumeToken(TOKEN_IDENTIFIER, "Expect superclass method name");
        uint8_t name = identifierConstant(&parser.previous);
        
        namedVariable(syntheticToken("this"), false);

        if (match(TOKEN_LEFT_PAREN)) {
            uint8_t argCount = argumentList();
            namedVariable(syntheticToken("super"), false);
            emitBytes(OP_SUPER_INVOKE, name);
            emitByte(argCount);
        } 
        else {
            namedVariable(syntheticToken("super"), false);
            emitBytes(OP_GET_SUPER, name);
        }
    }
    
    static void this_(bool canAssign) {
        if (currentClass == NULL) {
            parseError("Can't use 'this' outside of a class");
            return;
        }
        
        handleVariables(false);
    } 

    static void unaryOps(bool canAssign) {
        TokenType operatorType = parser.previous.type;

        parserPrecedence(PREC_UNARY);

            switch (operatorType) {
                case TOKEN_BANG: emitByte(OP_NOT); break;
                case TOKEN_MINUS: emitByte(OP_NEGATE); break;
                default: 
                    return;
            }
    }
    
    //dict of parse rules to assign each type of token to a certain order of precedence
    LoxParsePrecRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {handleGrouping, call,   PREC_CALL},
        [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
        [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
        [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
        [TOKEN_MINUS]         = {unaryOps, binaryOps, PREC_TERM},
        [TOKEN_PLUS]          = {NULL,     binaryOps, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
        [TOKEN_SLASH]         = {NULL,     binaryOps, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL,     binaryOps, PREC_FACTOR},
        [TOKEN_BANG]          = {unaryOps,    NULL,   PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL,     binaryOps, PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL,     binaryOps, PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL,     binaryOps, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL,     binaryOps, PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL,     binaryOps, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL,     binaryOps, PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {handleVariables, NULL,   PREC_NONE},
        [TOKEN_STRING]        = {handleStr,   NULL,   PREC_NONE},
        [TOKEN_NUMBER]        = {handleNumbers,   NULL,   PREC_NONE},
        [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
        [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FALSE]         = {handleLiterals,  NULL,   PREC_NONE},
        [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
        [TOKEN_NIL]           = {handleLiterals,  NULL,   PREC_NONE},
        [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
        [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
        [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},
        [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
        [TOKEN_TRUE]          = {handleLiterals,  NULL,   PREC_NONE},
        [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
    };
    
    static void parserPrecedence(LoxPrecedence precedence) {
        advance();

        ParseFunc prefixRule = getRule(parser.previous.type)->prefix;

        if (prefixRule == NULL) {
            parseError("Expect expression");
            return;
}

        bool canAssign = precedence <= PREC_ASSIGNMENT;
        prefixRule(canAssign);

        while (precedence <= getRule(parser.current.type)->precedence) {
            advance();
            ParseFunc infixRule = getRule(parser.previous.type)->infix;
            infixRule(canAssign);
        }

        if (canAssign && match(TOKEN_EQUAL)) {
            parseError("Invalid assignment target.");
        }
 
    }
   
    static LoxParsePrecRule* getRule(TokenType type) {
        return &rules[type];
    }

    static void handleExpression() {
        parserPrecedence(PREC_ASSIGNMENT);
    }

    static void block() {
        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
            handleDeclaration();
        }

        consumeToken(TOKEN_RIGHT_BRACE, "Expect '}' after block");
    }
   
    static void function(FunctionType type) {
        LoxCompiler compiler;
        initCompiler(&compiler, type);
        //create new scope for function variables, expressions, etc
        beginScope(); 

        consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
        if (!check(TOKEN_RIGHT_PAREN)) {
            do {
            current->function->arity++;
            if (current->function->arity > 255) {
                parseErrorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
            } while (match(TOKEN_COMMA));
        }
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        consumeToken(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        block();

        LoxObjFunction* function = endCompiler();

        emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

        for (int i = 0; i < function->upvalueCount; i++) {
            emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
            emitByte(compiler.upvalues[i].index);
        }

}
    static void handleMethods() {
        consumeToken(TOKEN_IDENTIFIER, "Expect method name.");
        uint8_t constant = identifierConstant(&parser.previous);

        FunctionType type = TYPE_METHOD;

        if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
            type = TYPE_INITIALIZER;
        }
        
        function(type);
        emitBytes(OP_METHOD, constant);
    }
 
    static void classDeclaration() {
        consumeToken(TOKEN_IDENTIFIER, "Expect class name");
        LoxToken className = parser.previous;
        uint8_t nameConstant = identifierConstant(&parser.previous);
        declareVariable();

        emitBytes(OP_CLASS, nameConstant);
        defineVariable(nameConstant);

        ClassCompiler classCompiler;
        classCompiler.hasSuperclass = false;
        classCompiler.enclosing = currentClass;
        currentClass = &classCompiler;

        if (match(TOKEN_LESS)) {
            consumeToken(TOKEN_IDENTIFIER, "Expect superclass name");
            handleVariables(false);

            if (identifiersEqual(&className, &parser.previous)) {
                parseError("A class can't inherit from itself");
            }

            beginScope();
            addLocal(syntheticToken("super"));
            defineVariable(0);
            
            namedVariable(className, false);
            emitByte(OP_INHERIT);
            classCompiler.hasSuperclass = true;
        }
        
        namedVariable(className, false);

        consumeToken(TOKEN_LEFT_BRACE, "Expect '{' before class body");

        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
            handleMethods();
        }

        consumeToken(TOKEN_RIGHT_BRACE, "Expect '}' after class body");
        emitByte(OP_POP);

        if (classCompiler.hasSuperclass) {
            endScope();
        }

        currentClass = currentClass->enclosing;
    }
    
    static void funDeclaration() {
        uint8_t global = parseVariable("Expecting function name");
        markInitialized();
        function(TYPE_FUNCTION);
        defineVariable(global);
    }

    static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");
    if (match(TOKEN_EQUAL)) {
        handleExpression();
    } 
    else {
        emitByte(OP_NIL);
    }
    consumeToken(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");
    defineVariable(global);
    }

    static void expressionStatement() {
        handleExpression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after expression.");
        emitByte(OP_POP);
    }

    //handle the possible tokens/environments within for loops
    static void forStatement() {
        //open new scope for the new vars, expressions of the for loop
        beginScope();

        consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
        if (match(TOKEN_SEMICOLON)) {
            // for(;;), nothing important
        } 
        else if (match(TOKEN_VAR)) {
            varDeclaration();
        } 
        else {
            expressionStatement();
        }

        int loopStart = currentChunk()->count;

        int exitJump = -1;
        if (!match(TOKEN_SEMICOLON)) {
            handleExpression();
            consumeToken(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

            exitJump = emitJump(OP_JUMP_IF_FALSE);
            emitByte(OP_POP); 
        }

        if (!match(TOKEN_RIGHT_PAREN)) {
            int bodyJump = emitJump(OP_JUMP);
            int incrementStart = currentChunk()->count;
            handleExpression();
            emitByte(OP_POP);
            consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

            emitLoop(loopStart);
            loopStart = incrementStart;
            patchJump(bodyJump);
        }

        handleStatement();
        emitLoop(loopStart);

        if (exitJump != -1) {
            patchJump(exitJump);
            emitByte(OP_POP); 
        }

        //close the current scope for the for loop's variables, expressions, etc
        endScope();
}
  
    static void ifStatement() {
        consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        handleExpression();
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

        int thenJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        handleStatement();

        int elseJump = emitJump(OP_JUMP);

        patchJump(thenJump);
        emitByte(OP_POP);

        if (match(TOKEN_ELSE)) handleStatement();
        patchJump(elseJump);
    }
        
    static void printStatement() {
        handleExpression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after value.");
        emitByte(OP_PRINT);
        }
    
        static void returnStatement() {
        if (current->type == TYPE_SCRIPT) {
            parseError("Can't return from top-level code.");
        }

        //< return-from-script
        if (match(TOKEN_SEMICOLON)) {
            emitReturn();
        } else {
            if (current->type == TYPE_INITIALIZER) {
                parseError("Can't return a value from an initializer.");
            }

            handleExpression();
            consumeToken(TOKEN_SEMICOLON, "Expect ';' after return value.");
            emitByte(OP_RETURN);
        }
    }
    static void whileStatement() {
        int loopStart = currentChunk()->count;
        consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
        handleExpression();
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int exitJump = emitJump(OP_JUMP_IF_FALSE);

        emitByte(OP_POP);
        handleStatement();
        emitLoop(loopStart);
        patchJump(exitJump);
        emitByte(OP_POP);

    }
    static void synchronize() {
        parser.resync = false;

        while (parser.current.type != TOKEN_EOF) {
            if (parser.previous.type == TOKEN_SEMICOLON) return;
            switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ; 
            }
            advance();
        }
    }

static void handleDeclaration() {
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } 
    else if (match(TOKEN_FUN)) {
        funDeclaration();
    } 
    else if (match(TOKEN_VAR)) {
        varDeclaration();
    } 
    else {
        handleStatement();
    }
        //similar to the sync() from c#
        //use the resync bool since we have no exceptions
        if (parser.resync) synchronize();
    }

    static void handleStatement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } 
    else if (match(TOKEN_FOR)) {
        forStatement();
    } 
    else if (match(TOKEN_IF)) {
        ifStatement();
    } 
    else if (match(TOKEN_RETURN)) {
        returnStatement();
    } 
    else if (match(TOKEN_WHILE)) {
        whileStatement();
    } 
    else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
    }

    LoxObjFunction* compileCode(const char* sourceCode) {
    initScanner(sourceCode);
    LoxCompiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    //reset errors for compiler processing
    parser.hadError = false;
    parser.resync = false;

    advance();

    while (!match(TOKEN_EOF)) {
        handleDeclaration();
    }

    LoxObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
    }
    void markCompilerRoots() {
    LoxCompiler* compiler = current;
    while (compiler != NULL) {
        markObject((LoxObject*)compiler->function);
        compiler = compiler->enclosing;
    }
    }
