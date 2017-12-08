/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   code.h
 * Author: pintos
 *
 * Created on December 5, 2017, 11:51 PM
 */

#ifndef CODE_H
#define CODE_H

#include "threads/interrupt.h"
bool create_handler( struct intr_frame *f);
int open_handler( struct intr_frame *f);
struct file * fd_to_file(int fd);
void close_handler( struct intr_frame *f);
int filesize_handler( struct intr_frame *f);
void exec_handler( struct intr_frame *f);
int wait_handler( struct intr_frame *f);
int read_handler( struct intr_frame *f);

#endif /* CODE_H */

