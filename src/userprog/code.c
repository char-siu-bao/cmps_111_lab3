/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


static void
push_command_helper(const char *cmdline UNUSED, void **esp){
 
    char *cmdline_copy = NULL;
    cmdline_copy = palloc_get_page(0);
    strlcpy(cmdline_copy, cmdline, PGSIZE);
    
    char *cmdline_copy2 = NULL;
    cmdline_copy2 = palloc_get_page(0);
    strlcpy(cmdline_copy2, cmdline, PGSIZE);
    
//    find argc 
    char *token;
    char *rest = cmdline_copy;
    int argc = 0;
    token = strtok_r(rest, " ", &rest);
    while (token != NULL){
        token = strtok_r(NULL, " ", &rest);
        argc++;
    }
//    end of find argc
    
//    store cmdline to argv[]
    char *token1;
    char *rest1 = cmdline_copy2;
    char * argv[argc];
    int argvInd = 0;
    token1 = strtok_r(rest1, " ", &rest1);
    while (token1 != NULL){
        argv[argvInd] = token1;
        argvInd++;
        token1 = strtok_r(NULL, " ", &rest1);
    }
//    end of store cmdline to argv[]
//    push argv[] to esp and store address to argvAddress[]
    int argvAddress[argc];
    for (int i = argc -1; i >= 0; i--){
        int len = strlen(argv[i]) + 1;
        *esp -= len;
        argvAddress[i] = (int)memcpy(*esp, argv[i], len);
    }
//    end of push argv[] to esp and store address to argvAddress[]
    
//    word alignment
    *esp = (void*) ((unsigned int) (*esp) & 0xfffffffc);
    
//    ending argv adress
    *esp -= 4;
    *((int*) *esp) = 0;
    
//    pushing argv address to stack
    for (int i = argc -1; i >= 0; i--){
        *esp -= 4;
        *((int*) *esp) = argvAddress[i];
    }
//    end of pushing argv address to stack
    int argvBase = (int)*esp;
    *esp -= 4;
    *((int*) *esp) = argvBase;
    *esp -= 4;
    *((int*) *esp) = argc;
//    return fake return address
    *esp -= 4;
    *((int*) *esp) = 0;
    
}