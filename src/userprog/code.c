/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <syscall-nr.h>
#include <list.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <round.h>

#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/umem.h"
#include "userprog/code.h"

bool 
create_handler( struct intr_frame *f){
    char *file;
    unsigned initial_size;
    
    umem_read(f->esp + 4, &file, sizeof(file));
    umem_read(f->esp + 8, &initial_size, sizeof(initial_size));
    
    bool create_bool = filesys_create(file, initial_size, false);
    f->eax = create_bool;
    return create_bool;
}
int 
open_handler( struct intr_frame *f){
    char *file;
    int open_val = -1;
    umem_read(f->esp + 4, &file, sizeof(file));
    struct file *file_ret = filesys_open(file);
    
    if (file_ret != NULL){
        list_push_back(&thread_current()->fds, &file_ret->elem);
        open_val = list_size(&thread_current()->fds) + FD_STDOUT;
    }
    f->eax = open_val;
    return open_val;
}

struct file * 
fd_to_file(int fd){
    int fd_check = FD_STDOUT;
    struct file *filefd = NULL;
    struct list_elem *e;

    for (e = list_begin (&thread_current()->fds); 
        e != list_end (&thread_current()-> fds);
        e = list_next (e))
        {
          fd_check ++;
          if(fd_check == fd){
              filefd = list_entry(e, struct file, elem);
          }
        }
        
    return filefd; 
}
void 
close_handler( struct intr_frame *f){
    int fd;
    umem_read(f->esp +4, &fd, sizeof(fd));
    struct file *filefd = fd_to_file(fd);
    
    file_close(filefd);
    f->eax = 0;
}
int
filesize_handler( struct intr_frame *f){
    int fd;
    umem_read(f->esp +4, &fd, sizeof(fd));
    struct file *filefd = fd_to_file(fd);
    
    int filesize = file_length(filefd);
    f->eax = filesize;
    return filesize;
       
}
void 
exec_handler( struct intr_frame *f){
    char * cmdline;
    umem_read(f->esp +4, &cmdline, sizeof(cmdline));
    
    f->eax =  process_execute(cmdline); 
}
int
wait_handler( struct intr_frame *f){
    int child_pid;
    umem_read(f->esp +4, &child_pid, sizeof(child_pid));
    int wait_val = process_wait(child_pid);
    f->eax = wait_val;
    return wait_val;
}
int
read_handler( struct intr_frame *f){
    int fd;
    void *buffer;
    int read_val = 0;
    unsigned size;
    
    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));
    
    if (((unsigned)list_size(&thread_current()->fds))>= (fd - FD_STDOUT)){
        struct file *filefd = fd_to_file(fd);
        read_val = file_read(filefd, buffer, size);
    }
    
    f->eax = read_val;
    return read_val;
   
}
void
push_command_helper(const char *cmdline UNUSED, void **esp){
  
//    find argc 
    char *cmdline_copy = NULL;
    cmdline_copy = palloc_get_page(0);    
    strlcpy(cmdline_copy, cmdline, PGSIZE);
    int argc = 0;
    for(char *token = strtok_r(cmdline_copy, " ", &cmdline_copy);
        token != NULL;
        argc++, 
        token = strtok_r(NULL, " ", &cmdline_copy)){}

//    end of find argc
    
//    store cmdline to argv[]
    char *cmdline_copy2 = NULL;
    cmdline_copy2 = palloc_get_page(0);    
    strlcpy(cmdline_copy2, cmdline, PGSIZE);
    char * argv[argc];
    int argvInd = 0;
    for(char *token1 = strtok_r(cmdline_copy2, " ", &cmdline_copy2 );
        token1 != NULL;
        argvInd++,
        token1 = strtok_r(NULL, " ", &cmdline_copy2)
        ){argv[argvInd] = token1;}

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