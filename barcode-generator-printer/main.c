#include<stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

/* reads in barcodes  */


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


void print_barcode (int year, char *filler, long belnr) {
    printf("Barcode wird gedruckt: %i%s%ld\n", year, filler, belnr);
    
    // TODO remove this
    return;
    
    // generate barcode
    
    char barcode_cmd[100];
    sprintf(barcode_cmd, "barcode -b %i%s%ld -o out.ps -e \"i25\" -g \"590x300\";", year, filler, belnr);
    system(barcode_cmd);
    
    // convert .ps to .png with graphicsmagick
    
    system("gm convert out.ps -gravity south -resize 80% -extent 696x271 out.png");
    
    // print with label printer Brother QL-720NW
    
    system("brother_ql_create --model QL-720NW --label-size 62x29 --no-cut out.png > /dev/usb/lp0");
    
}


int main (void) {
    printf("Dieses Programm erlaubt das Drucken von Barcodes.\n");
    
    int year = read_year_from_file();
    printf("JAHR: %i\n", year);
    
    char * x = (char*)malloc(10);
    
    while (true) {
        scanf("%s", x);
        char *end;
        long in = strtol(x, &end, 10);
        
        if ((in >= 5000 && in <= 9999)) {
            // RANGE: 5000-9999
            print_barcode(year, "00000", in);
            
        } else if ((in >= 100000 && in <= 199999)) {
            // RANGE: 100000-199999
            print_barcode(year, "000", in);
            
        } else if ((in >= 2014 && in <= 2099)) {
            printf("JAHR gesetzt: %ld \n", in);
            year = in;
            write_year_to_file(year);
            
        } else {
            printf("UNGÜLTIG\n");
        }
        
    }
    
    return 0;
}

