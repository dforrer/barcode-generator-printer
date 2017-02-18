#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <regex.h>  

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


void print_barcode (int year, long belnr) {
    printf("Barcode wird gedruckt: %i%09ld\n", year, belnr);
    
    // TODO remove this
    return;
    
    // generate barcode
    
    char barcode_cmd[100];
    sprintf(barcode_cmd, "barcode -b %i%09ld -o out.ps -e \"i25\" -g \"590x300\";", year, belnr);
    system(barcode_cmd);
    
    // convert .ps to .png with graphicsmagick
    
    system("gm convert out.ps -gravity south -resize 80% -extent 696x271 out.png");
    
    // print with label printer Brother QL-720NW
    
    system("brother_ql_create --model QL-720NW --label-size 62x29 --no-cut out.png > /dev/usb/lp0");
    
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


int main (void) {
    printf("Dieses Programm erlaubt das Drucken von Barcodes.\n");
    
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
        scanf("%s", x);

        /* Execute regular expression */

        if ( regex_matches(regex_5000_9999, x) ||
             regex_matches(regex_100000_199999, x) ) {
            printf("5000/100000 match!\n");
            char *end;
            long in = strtol(x, &end, 10);
            print_barcode(year, in);
            belnr = in;
        }
        else if ( regex_matches(regex_range_5000_9999, x) ) {
            printf("5000-9999 match!\n");
            char *end;
            long from = strtol( substring(x, 0, 4), &end, 10);
            long to   = strtol( substring(x, 5, 4), &end, 10);
            
            if ( from > to ) {
                continue;
            }
            
            for ( long i = from; i <= to; i++ ) {
                print_barcode(year, i);
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
            
            for ( long i = from; i <= to; i++ ) {
                print_barcode(year, i);
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
            print_barcode(year, belnr);
        }
        else {
            printf("UNGÜLTIG\n");
        }
    }
    
    /* Free memory allocated to the pattern buffer by regcomp() */
    //regfree(&regex_5000_9999);
    // regfree(&regex_100000_199999);
    
    return 0;
}

