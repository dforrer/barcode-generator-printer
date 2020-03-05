# Barcode generator printer

## How to compile and run

```
$ gcc -pthread main.c pipe.c -o barcode_printer
$ ./barcode_printer
```

## Features

- Allows Fast-input by adding inputs to a create queue and later print queue

## Multithreading

```
create_barcode () => 1000-2017-100001.png
print_label ()

Create Barcode Queue (thread 1)		Print Queue (thread 2)
- bc5003 (calculating)				- bc5001 (printing)
- bc5004 (calculating)				- bc5002 (queued)
- bc5005 (queued)
- bc5006 (queued)
```


## Program listens for keyboard input (scanf)

**CHANGE YEAR**

```
Keyboard entry:
2016 => sets year to new value
```

Input-Validation:
- Numeric value is in the range 2014-2099

**SINGLE-MODE**

```
Keyboard entry:
5000   + ENTER => Prints Barcode 1000-2017-5000
100000 + ENTER => Prints Barcode 1000-2017-100000
```

Input-Validation:
- Whitelist numbers 0-9
- Range 5000-9999
- Range 100000-199999

**RANGE-MODE**

```
Keyboard entry:
5000-5002 + ENTER => Prints Barcodes 1000-2017-5000, 1000-2017-5001, 1000-2017-5002
```

**PLUS-MODE**

```
Keyboard entry:
"+" + ENTER => Prints Barcode last barcode + 1
```

**CHANGE COMPANY CODE**

Only two company codes are accepted: 1000 and 5000

```
Keyboard entry:
"*" + 1000 => sets company code to 1000
"*" + 5000 => sets company code to 5000
```
