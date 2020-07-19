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


void gen_random(char *s, const int len) {
    static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
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

int write_company_code_to_file (int company_code) {

    FILE *f = fopen("company_code.txt", "w");
    if (f == NULL) {

        printf("Error opening file!\n");
        return 0;

    }

    /* print integers and floats */
    fprintf(f, "%d\n", company_code);

    fclose(f);
    return 1;
}


int read_year_from_file () {

    FILE *f = fopen("year.txt", "r");
    if (f == NULL) {

        printf("Error opening file!\n");
        return 2020;

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

int read_company_code_from_file () {

    FILE *f = fopen("company_code.txt", "r");
    if (f == NULL) {

        printf("Error opening file!\n");
        return 1000;

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
        fprintf(stderr, "Could not compile regex_single_number\n");
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

struct Barcode {
    char * str;
    int company_code;
    int year;
    long long belnr;
    char * filename;
    int format_version;
};


void create_barcode ( struct Barcode * bc) {
    printf("create_barcode() = %s\n", bc->str);

#ifdef TEST
    return;
#endif

    char barcode_cmd[200];
    char convert_cmd[300];
    if ( bc->format_version == 0 ) {
        // generate barcode
        sprintf(barcode_cmd, "barcode -b %s -o %s.ps -e \"i25\" -g \"590x300\";", bc->str, bc->filename);
        // convert .ps to .png with graphicsmagick
        sprintf(convert_cmd, "gm convert %s.ps -gravity south -resize 80%% -extent 696x271 -rotate 180 %s.png", bc->filename, bc->filename);
    } else {
        // generate barcode
        sprintf(barcode_cmd, "barcode -b %s -o %s.ps -e \"128b\" -g \"600x240\";", bc->str, bc->filename);
        // convert .ps to .png with graphicsmagick
        sprintf(convert_cmd, "gm convert %s.ps -gravity south -extent 696x271 -rotate 180 -fill white -draw 'rectangle 80,33,618,44' -fill none -stroke black -strokewidth 2 -draw 'rectangle 395,4,508,38' %s.png", bc->filename, bc->filename);
    }
    system(barcode_cmd);
    system(convert_cmd);
}


void print_label ( struct Barcode * bc ) {
    printf("print_label: %s\n", bc->str);

#ifdef TEST
    return;
#endif
    // print with label printer Brother QL-720NW

    char print_cmd[200];
    sprintf(print_cmd, "brother_ql_create --model QL-720NW --label-size 62x29 -c 1 --no-cut %s.png > /dev/usb/lp0", bc->filename);

    if ( access( "/dev/usb/lp0", W_OK ) != -1 ) {
        //printf("/dev/usb/lp0 exists!\n");
        system(print_cmd);
    } else {
        printf("/dev/usb/lp0 doesn't exist!\n");
    }
}


/* 500020190000100999 */
void print_barcode (pipe_producer_t* pipe_creator_prod, int company_code, int year, long long belnr, int format_version) {
    if (belnr > 9999999999) {
        return;
    }
    char * barcode = malloc(20+1);
    char * filename = malloc(200);
    char rand[10+1];
    gen_random(rand, 10);
    if ( format_version == 0 ) {
        sprintf(barcode, "%i%09lld", year, belnr);
    } else {
        sprintf(barcode, "%i%i%010lld", company_code, year, belnr);
    }
    sprintf(filename,"%s_%s",barcode, rand);

    printf("filename: %s\n",filename);
    struct Barcode bc = {barcode, company_code, year, belnr, filename, format_version};
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
        create_barcode(&bc);
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
        print_label(&bc);
        char sys_call [40];
#ifdef TEST
        continue;
#endif
        sprintf(sys_call, "rm %s.*", bc.filename);
        system(sys_call);
        free(bc.str);
        free(bc.filename);
    }
    return NULL;
}

int run () {
    printf("This program allows you to print barcodes.\n");

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
    int format_version = 1;
    int company_code = read_company_code_from_file();
    int year  = read_year_from_file();
    long long belnr = 5000;
    printf("year: %i\n", year);

    char * x = (char*)malloc(20);

    // Prepare regex
    regex_t regex_single_number  = compile_regex("^[0-9]{1,10}$");
    regex_t regex_range          = compile_regex("^[0-9]{1,10}-[0-9]{1,10}$");
    regex_t regex_plus           = compile_regex("^\\+$"); // e.g. +
    regex_t regex_year           = compile_regex("^\\/2[0-9]{3}$"); // e.g. /2021
    regex_t regex_company_code   = compile_regex("^\\*[0-9]{4}$"); // e.g. *1000
    regex_t regex_format_version = compile_regex("^\\*\\*[0-9]\\*\\*$"); // e.g. **0**
    regex_t regex_restart        = compile_regex("^\\.\\.\\.$"); // ...

    while (true) {
        scanf("%s", x); // blocking

        /* Execute regular expression */

        if ( regex_matches(regex_single_number, x) ) {
            printf("regex_single_number match!\n");
            char *end;
            belnr = strtoll(x, &end, 10);
            print_barcode(pipe_creator_prod, company_code, year, belnr, format_version);
        }
        else if ( regex_matches(regex_range, x) ) {
            printf("regex_range match!\n");
            char * first_num = strtok(x, "-");
            char * second_num = strtok(NULL, " ");
            char *end;
            long long from = strtoll( first_num, &end, 10);
            long long to   = strtoll( second_num, &end, 10);
            if ( from > to ) {
              // flip from and to
              long long from_tmp = from;
              from = to;
              to = from_tmp;
            }
            // limit number of barcodes printed to 100
            if ( (to - from) > 200 ) {
              continue;
            }
            long long belnr_i = 0;
            for ( belnr_i = from; belnr_i <= to; belnr_i++ ) {
                print_barcode(pipe_creator_prod, company_code, year, belnr_i, format_version);
            }
            belnr = to;
        }
        else if ( regex_matches(regex_year, x) ) {
            char *end;
            long in = strtol( substring(x, 1, 4), &end, 10);
            printf("year set to: %ld \n", in);
            year = (int) in;
            write_year_to_file(year);
        }
        else if ( regex_matches(regex_plus, x) ) {
            printf("Inkrement\n");
            belnr++;
            print_barcode(pipe_creator_prod, company_code, year, belnr, format_version);
        }
        else if ( regex_matches(regex_company_code, x) ) {
            printf("Company code\n");
            char *end;
            long cc = strtol( substring(x, 1, 4), &end, 10);
            printf("company_code set to: %ld \n", cc);
            company_code = (int) cc;
            write_company_code_to_file(company_code);
        }
        else if ( regex_matches(regex_format_version, x) ) {
            printf("format_version mode\n");
            char *end;
            long in = strtol( substring(x, 2, 1), &end, 10);
            printf("format_version set to: %ld \n", in);
            format_version = (int) in;
        }
        else if ( regex_matches(regex_restart, x) ) {
            printf("restart app\n");
            /* kill creator_thread */
            void *res;
            int s;
            s = pthread_cancel(creator_thread);
            if (s != 0) {
               printf("pthread_cancel error: %d\n", s);
            }
            s = pthread_join(creator_thread, &res);
            if (s != 0) {
                printf("pthread_join error: %d\n", s);
            }
            if (res == PTHREAD_CANCELED) {
                printf("main(): thread was canceled\n");
            } else {
                printf("main(): thread wasn't canceled (shouldn't happen!)\n");
            }
            /* kill printer_thread */
            s = pthread_cancel(printer_thread);
            if (s != 0) {
               printf("pthread_cancel error: %d\n", s);
            }
            s = pthread_join(printer_thread, &res);
            if (s != 0) {
                printf("pthread_join error: %d\n", s);
            }
            if (res == PTHREAD_CANCELED) {
                printf("main(): thread was canceled\n");
            } else {
                printf("main(): thread wasn't canceled (shouldn't happen!)\n");
            }
            /* Free memory allocated to the pattern buffer by regcomp() */
            regfree(&regex_single_number);
            regfree(&regex_range);
            regfree(&regex_plus);
            regfree(&regex_year);
            regfree(&regex_company_code);
            regfree(&regex_format_version);
            regfree(&regex_restart);

            /* cleanup generated barcodes */
            system("rm *.png");
            system("rm *.ps");

            // pipe_producer_free(pipe_creator_prod);
            // pipe_consumer_free(pipe_creator_cons);
            // pipe_producer_free(pipe_printer_prod);
            // pipe_consumer_free(pipe_printer_cons);

            free(x);
            return 0;
        }
        else {
            printf("---INVALID---\n");
        }
    }

    return 0;
}

int main (void) {
    while( true ) {
        run();
    }
}
