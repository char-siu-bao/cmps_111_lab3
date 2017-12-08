/* 
 * This file is derived from source code for the Pintos
 * instructional operating system which is itself derived
 * from the Nachos instructional operating system. The 
 * Nachos copyright notice is reproduced in full below. 
 *
 * Copyright (C) 1992-1996 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose, without fee, and
 * without written agreement is hereby granted, provided that the
 * above copyright notice and the following two paragraphs appear
 * in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
 * AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *
 * Modifications Copyright (C) 2017 David C. Harrison. All rights reserved.
 * 
 * pintos -v -k -T 60 --qemu --filesys-size=2 -p build/tests/userprog/read-normal -a read-normal -p ../tests/userprog/sample.txt -a sample.txt -- -q -f run read-normal
 * 
 * pintos -v -k -T 60 --qemu --filesys-size=2 -p build/tests/userprog/open-normal -a open-normal -p ../tests/userprog/sample.txt -a sample.txt -- -q -f run open-normal

 */

#include <stdio.h>
#include <syscall-nr.h>
#include <list.h>

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

//put into code.c
static void create_handler( struct intr_frame *f){
    char *file;
    unsigned initial_size;
    
    umem_read(f->esp + 4, &file, sizeof(file));
    umem_read(f->esp + 8, &initial_size, sizeof(initial_size));
    
    f->eax = filesys_create(file, initial_size, false);
    
}
static int open_handler( struct intr_frame *f){
    char *file;
    umem_read(f->esp + 4, &file, sizeof(file));
    struct file *file_ret = filesys_open(file);
    if (file_ret == NULL){
        f->eax = -1;
    }
    else{
        list_push_back(&thread_current()->fds, &file_ret->elem);
        f->eax = list_size(&thread_current()->fds) + 2;
    }

}



struct file * 
fd_to_file(int fd){
    int fd_check = 2;
    struct file *filefd;
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
static void close_handler( struct intr_frame *f){
    int fd;
    umem_read(f->esp +4, &fd, sizeof(fd));
//    fd = thread_current()->read_fd;
    struct file *filefd = fd_to_file(fd);
    
    file_close(filefd);
    f->eax = 0;
       
}

static void filesize_handler( struct intr_frame *f){
    int fd;
    umem_read(f->esp +4, &fd, sizeof(fd));
    struct file *filefd = fd_to_file(fd);
    
    f->eax = file_length(filefd);
       
}

static void exec_handler( struct intr_frame *f){
    char * cmdline;
    umem_read(f->esp +4, &cmdline, sizeof(cmdline));
    
    f->eax =  process_execute(cmdline); 
}


static void read_handler( struct intr_frame *f){
    int fd;
    void *buffer;
    unsigned size;
    
    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));
    
    if (list_size(&thread_current()->fds) < fd - 2){
        f->eax = 0;
    }
    else{
        struct file *filefd = fd_to_file(fd);
        f->eax = file_read(filefd, buffer, size);
    }
   
}

static void syscall_handler(struct intr_frame *);

static void write_handler(struct intr_frame *);
static void exit_handler(struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f)
{
  int syscall;
  ASSERT( sizeof(syscall) == 4 ); // assuming x86

  // The system call number is in the 32-bit word at the caller's stack pointer.
  umem_read(f->esp, &syscall, sizeof(syscall));

  // Store the stack pointer esp, which is needed in the page fault handler.
  // Do NOT remove this line
  thread_current()->current_esp = f->esp;

  switch (syscall) {
  case SYS_HALT: 
    shutdown_power_off();
    break;

  case SYS_EXIT: 
    exit_handler(f);
    break;
      
  case SYS_WRITE: 
    write_handler(f);
    break;
    
  case SYS_CREATE:
    create_handler(f);
    break;
    
  case SYS_OPEN:
    open_handler(f);
    break;
    
  case SYS_READ:
    read_handler(f);
    break;
    
  case SYS_FILESIZE:
    filesize_handler(f);
    break;
    
  case SYS_CLOSE:
    close_handler(f);
    break;
   
//  case SYS_EXEC:
//    exec_handler(f);
//    break;
    
  case SYS_WAIT:
    break;
    
  default:
    printf("[ERROR] system call %d is unimplemented!\n", syscall);
    thread_exit();
    break;
  }
}

/****************** System Call Implementations ********************/

// *****************************************************************
// CMPS111 Lab 3 : Put your new system call implementatons in your 
// own source file. Define them in your header file and include 
// that .h in this .c file.
// *****************************************************************

void sys_exit(int status) 
{
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

static void exit_handler(struct intr_frame *f) 
{
  int exitcode;
  umem_read(f->esp + 4, &exitcode, sizeof(exitcode));

  sys_exit(exitcode);
}

/*
 * BUFFER+0 and BUFFER+size should be valid user adresses
 */
static uint32_t sys_write(int fd, const void *buffer, unsigned size)
{
  umem_check((const uint8_t*) buffer);
  umem_check((const uint8_t*) buffer + size - 1);

  int ret = -1;

  if (fd == 1) { // write to stdout
    putbuf(buffer, size);
    ret = size;
  }
  
  else if (fd > 2){
      struct file *filefd = fd_to_file(fd);
      ret = file_write(filefd, buffer, size);
  }
  return (uint32_t) ret;
}

static void write_handler(struct intr_frame *f)
{
    int fd;
    const void *buffer;
    unsigned size;

    umem_read(f->esp + 4, &fd, sizeof(fd));
    umem_read(f->esp + 8, &buffer, sizeof(buffer));
    umem_read(f->esp + 12, &size, sizeof(size));

    f->eax = sys_write(fd, buffer, size);
}


