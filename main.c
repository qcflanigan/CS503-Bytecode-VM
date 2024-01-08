#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "common.h"
#include "LoxChunk.h"
#include "LoxDebugger.h"
#include "LoxVM.h"


//execute line by line from terminal input
static void ExecutePrompt() {
    char input[1024];
    for (;;) {
        printf("Lox> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
    }
        interpretCode(input);
}
}

static char* readFile(const char* path) {
    FILE* LoxFile = fopen(path, "rb");
    if (LoxFile == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    fseek(LoxFile, 0L, SEEK_END);
    size_t fileSize = ftell(LoxFile);
    rewind(LoxFile);
    char* buffer = (char*)malloc(fileSize + 1);
    //not enough space to hold the size of the Lox code file
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t fileDataSize = fread(buffer, sizeof(char), fileSize, LoxFile);
    //read fails
    if (fileDataSize < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[fileDataSize] = '\0';
    fclose(LoxFile);
return buffer;
}

static void ExecuteFile(const char* path) {
        char* code = readFile(path);
        InterpreterResult evaluatedCode = interpretCode(code);
        free(code);
        //comment out to allow for full testing without exiting after each error
        if (evaluatedCode == INTERPRET_COMPILE_ERROR); //exit(65);
        if (evaluatedCode == INTERPRET_RUNTIME_ERROR); //exit(70);
}


int main(int argc, const char* argv[]) {
    initLoxVM();
    
    if (argc == 1) {
        ExecutePrompt();
    } 
    else if (argc == 2) {
        ExecuteFile(argv[1]);
    } 
    else if (argc==3){
        int testCount=0;
        char *directories[] = {"assignment", "benchmark", "block", "bool", "call", "class", "closure", "comments", "constructor",
                                "expressions", "field", "for", "function", "if", "inheritance", "limit", "logical_operator", "method",
                                "nil", "number", "operator", "print", "regression", "return", "scanning", "string", "super", "T", "this",
                                "variable", "while"};

        //iterate through each test directory
        for (int i=0; i<sizeof(directories); i++){
            char *fullPath;
            char *path = "Testing\\test\\";
            fullPath[0] = '\0';
            //append each directory name to the Testing\test tag of the file structure
            strcat(fullPath, path);
            strcat(fullPath, directories[i]);
            //open each directory to grab each test lox file 
            DIR *directory = opendir(fullPath);

            if (directory==NULL){
                printf("Could not open directory");
                return 0;
            }

            struct dirent *entry;
            size_t arglen = strlen(fullPath);
            //iterate through each item/entry in the directory 
            while((entry=readdir(directory))!=NULL){
                
                //declare new string each loop for full file path with enough space for path+filename
                char *fullFilePath = malloc(arglen + strlen(entry->d_name) + 2);

                if (fullFilePath==NULL){
                    printf("malloc failed");
                    return 0;
                }
                //append the filename to the current directory path
                sprintf(fullFilePath, "%s\\%s", fullPath, entry->d_name);
                //check for the '.' and '..' files of each directory
                if (fullFilePath[strlen(fullFilePath)-1]=='.'){
                    //do nothing for '.' files
                }
                //execute each file
                else{
                    printf(fullFilePath);
                    printf("\n-------------------\n");
                    ExecuteFile(fullFilePath);
                    testCount++;
                    printf("\n");
                    if (strcmp(fullFilePath, "Testing\\test\\while\\var_in_body.lox")==0){
                        printf("%d", testCount);
                        return 0;
                    }
            }
                free(fullFilePath);

            }
            closedir(directory);
        }
    }
    else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
}
    freeLoxVM();

    return 0;

}

    
    
