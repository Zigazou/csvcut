csvcut
======

`csvcut` is a small command-line allowing you to cut some columns from a CSV
file.

The main goals of this utility are:

- speed
- memory efficiency (use less than 200 Kbytes)
- handling of very big CSV file
- low requirements
- handling of multi-line columns

Usage
-----

`csvcut` works only as a filter on standard input giving the result on standard
output. The following will keep only the first three columns of a CSV file using
semi-column as field delimiter: 

    cat input.csv | csvcut ';' 0 1 2 > output.csv

The first argument is always the delimiter (it must be provided).

The following arguments are the columns the user wants to keep. `csvcut` does
not care about the order you give the column numbers, it always returns the
columns in the source order. This allows `csvcut` to work on very big rows.

Using Bash, you can specify columns like this:

    cat input.csv | csvcut ';' {0..9} > output.csv

It will extract the first 10 columns to `output.csv`.

Limitations
-----------

`csvcut` can only output the first 131072 columns. If the CSV file contains more
columns, they are ignored. Processing nonetheless goes on without error.

If you really need to handle more columns, just change the `MAX_COLUMNS`
constant in `csvcut.c` and recompile the program.

Build
-----

You need the GNU C compiler though it should compile on a wide variety of
compiler.

    make
