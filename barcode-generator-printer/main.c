#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <regex.h>  
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "pipe.h"


/* reads in barcodes  */


char* substring (const char* in, int from, int size) {
    /* takes a const char array and returns a char* of size */
    if (from+size <= strlen(in)) {
        char *out = malloc(size+1);
        sprintf(out, "%.*s", size ,&in[from]);
        return out;
    } else {
        return NULL;
    }
}


int write_year_to_file (int year) {
    
    FILE *f = fopen("year.txt", "w");
    if (f == NULL) {
        
        printf("Error opening file!\n");
        return 0;
        
    }
    
    /* print integers and floats */
    fprintf(f, "%d\n", year);
    
    fclose(f);
    return 1;
}


int read_year_from_file () {
    
    FILE *f = fopen("year.txt", "r");
    if (f == NULL) {
        
        printf("Error opening file!\n");
        return 2016;
        
    }
    int rv = 0;
    char buf[4 + 1];
    size_t nread;
    
    if (f) {
        while ((nread = fread(buf, 1, sizeof buf, f)) > 0)
            sscanf(buf, "%d", &rv);
    }
    fclose(f);
    return rv;
}


regex_t compile_regex (char * regex) {
    regex_t r;
    int regex_rv;
    
    regex_rv = regcomp(&r, regex, REG_EXTENDED);
    if (regex_rv) {
        fprintf(stderr, "Could not compile regex_5000_9999\n");
        exit(1);
    }
    return r;
}


bool regex_matches (regex_t r, char * input) {
    
    int rv = regexec(&r, input, 0, NULL, 0);
    if (!rv) {
        return true;
    } else if (rv == REG_NOMATCH) {
        return false;
    }
    else {
        printf("Exiting because regex failed!");
        exit(1);
    }
    return false;
}


void create_barcode (char * barcode) {
    printf("create_barcode() = %s\n", barcode);

#ifdef TEST
    return;
#endif
    
    // generate barcode
    
    char barcode_cmd[100];
    sprintf(barcode_cmd, "barcode -b %s -o %s.ps -e \"i25\" -g \"590x300\";", barcode, barcode);
    system(barcode_cmd);
    
    // convert .ps to .png with graphicsmagick
    
    char convert_cmd[100];
    sprintf(convert_cmd, "gm convert %s.ps -gravity south -resize 80%% -extent 696x271 -rotate 180 %s.png", barcode, barcode);
    system(convert_cmd);
}


void print_label ( char * barcode ) {
    printf("print_label: %s\n", barcode);
    
#ifdef TEST
    return;
#endif
    // print with label printer Brother QL-720NW
    
    char print_cmd[200];
    sprintf(print_cmd, "brother_ql_create --model QL-720NW --label-size 62x29 -c 1 --no-cut %s.png > /dev/usb/lp0", barcode);
    
    if ( access( "/dev/usb/lp0", W_OK ) != -1 ) {
        printf("/dev/usb/lp0 exists!\n");
        system(print_cmd);
    } else {
        printf("/dev/usb/lp0 doesn't exist!\n");
    }
}


struct Barcode {
    char * str;
    int year;
    long belnr;
    char * filename;
};


void print_barcode (pipe_producer_t* pipe_creator_prod, int year, long belnr) {
    char * barcode = malloc(13+1);
    sprintf(barcode, "%i%09ld", year, belnr);
    struct Barcode bc = {barcode, year, belnr};
    pipe_push(pipe_creator_prod, &bc, 1);
}


struct Pipes {
    pipe_consumer_t *cons_pipe_creator;
    pipe_producer_t *prod_pipe_printer;
};


void * creator_thread_func (void *arg) {
    printf("creator_thread_func\n");
    struct Pipes * pipes = arg;
    pipe_consumer_t* pipe_creator_cons = pipes->cons_pipe_creator;
    while (true) {
        struct Barcode bc;
        (void) pipe_pop(pipe_creator_cons, &bc, 1); // blocking
        create_barcode(bc.str);
        pipe_push(pipes->prod_pipe_printer, &bc, 1);
    }
    return NULL;
}


void * printer_thread_func (void *arg) {
    printf("printer_thread_func\n");
    pipe_consumer_t* pipe_printer_cons = arg;
    while (true) {
        struct Barcode bc;
        (void) pipe_pop(pipe_printer_cons, &bc, 1);  // blocking
        print_label(bc.str);
        char sys_call [40];
#ifdef TEST
        continue;
#endif
        sprintf(sys_call, "rm %s.*", bc.str);
        system(sys_call);
        free(bc.str);
    }
    return NULL;
}


int main (void) {
    printf("Dieses Programm erlaubt das Drucken von Barcodes.\n");

    chdir("barcodes");
    
    // Creating pipes
    
    pipe_t* pipe_creator = pipe_new(sizeof(struct Barcode), 0);
    pipe_producer_t* pipe_creator_prod = pipe_producer_new(pipe_creator);
    pipe_consumer_t* pipe_creator_cons = pipe_consumer_new(pipe_creator);
    pipe_free(pipe_creator);
    
    pipe_t* pipe_printer = pipe_new(sizeof(struct Barcode), 0);
    pipe_producer_t* pipe_printer_prod = pipe_producer_new(pipe_printer);
    pipe_consumer_t* pipe_printer_cons = pipe_consumer_new(pipe_printer);
    pipe_free(pipe_printer);

    struct Pipes pipes;
    pipes.cons_pipe_creator = pipe_creator_cons;
    pipes.prod_pipe_printer = pipe_printer_prod;
    
    pthread_t creator_thread, printer_thread;
    
    if ( pthread_create(&creator_thread, NULL, creator_thread_func, &pipes) ) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    
    if ( pthread_create(&printer_thread, NULL, printer_thread_func, pipe_printer_cons) ) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    
    int year  = read_year_from_file();
    long belnr = 5000;
    printf("JAHR: %i\n", year);
    
    char * x = (char*)malloc(20);
    
    // Prepare regex
    regex_t regex_5000_9999             = compile_regex("^[5-9][0-9]{3}$");
    regex_t regex_100000_199999         = compile_regex("^1[0-9]{5}$");
    regex_t regex_range_5000_9999       = compile_regex("^[5-9][0-9]{3}-[5-9][0-9]{3}$");
    regex_t regex_range_100000_199999   = compile_regex("^1[0-9]{5}-1[0-9]{5}$");
    regex_t regex_plus                  = compile_regex("^\\+$");
    regex_t regex_year                  = compile_regex("^2[0-9]{3}$");
    
    
    while (true) {
        scanf("%s", x); // blocking

        /* Execute regular expression */

        if ( regex_matches(regex_5000_9999, x) ||
             regex_matches(regex_100000_199999, x) ) {
            printf("5000/100000 match!\n");
            char *end;
            belnr = strtol(x, &end, 10);
            print_barcode(pipe_creator_prod, year, belnr);
        }
        else if ( regex_matches(regex_range_5000_9999, x) ) {
            printf("5000-9999 match!\n");
            char *end;
            long from = strtol( substring(x, 0, 4), &end, 10);
            long to   = strtol( substring(x, 5, 4), &end, 10);
            
            if ( from > to ) {
                continue;
            }
            long i = 0;
            for ( i = from; i <= to; i++ ) {
                print_barcode(pipe_creator_prod, year, i);
            }
            
            belnr = to;
        }
        else if ( regex_matches(regex_range_100000_199999, x) ) {
            printf("100000-199999 match!\n");
            char *end;
            long from = strtol( substring(x, 0, 6), &end, 10);
            long to   = strtol( substring(x, 7, 6), &end, 10);
            
            if ( from > to ) {
                continue;
            }
            long i = 0;
            for ( i = from; i <= to; i++ ) {
                print_barcode(pipe_creator_prod, year, i);
            }
            
            belnr = to;
        }
        else if ( regex_matches(regex_year, x) ) {
            char *end;
            long in = strtol(x, &end, 10);
            printf("JAHR gesetzt: %ld \n", in);
            year = (int) in;
            write_year_to_file(year);
        }
        else if ( regex_matches(regex_plus, x) ) {
            printf("Inkrement\n");
            belnr++;
            print_barcode(pipe_creator_prod, year, belnr);
        }
        else {
            printf("UNGUELTIG\n");
        }
    }
    
    /* Free memory allocated to the pattern buffer by regcomp() */
    //regfree(&regex_5000_9999);
    //regfree(&regex_100000_199999);
    
    return 0;
}

