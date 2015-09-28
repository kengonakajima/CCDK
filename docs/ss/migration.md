Space Sweeper field data migration
====

## Overview
Space Sweeper saves field data in 16384 small compressed files in backend before v0.2.11,
but after v0.2.11, it saves the data in single uncompressed file (about 200MB).

You can convert old data to latest format with ssfind.rb and ssconv.

ssfind.rb is a Ruby script that scans old backend/datadir and find all projects saved in the directory.
It just only read and find projects, and no write by it.
ssconv just read old formatted files and write in a file in new format.

ssconv is built by games/ss/Makefile in Linux and ssfind.rb is just placed in games/ss.

Note that ssconv will only access static files and will not access Redis database at all.


## Conversion Steps

### 1. Backup all files in datadir and stop backend servers 


### 2. Find all projects with ssfind.rb

Command line example:

~~~
$ ruby ssfind.rb /Users/ringo/tmp/CCDK/backend/datadir
datadir: /Users/ringo/tmp/CCDK/backend/datadir
Project 1 : 16384 files found
Project 39 : 16384 files found
Project 4 : 16384 files found
~~~

ssfind.rb will scan for files that are named like "field_data_1_208_68_16_16".
If the project doesn't have less files, it means the project is partly corrupt.

### 3. Run ssconv
Build ssconv in games/ss with make command.

You can see usage of ssconv with no options

~~~
$ ./ssconv
Usage: ssconv DATADIR_IN DATADIR_OUT PROJID
~~~

DATADIR_IN is set to a old directory to read, DATADIR_OUT is set to a new directory to save. PROJID is project id to convert.

In new format, a field file is named like "field_data_4_2048_2048_48".
that means project id is 4, field size is 2048x2048, sizeof(Cell) is 48.

ssconv can only convert one project every time, you have to specify project id as the last parameter. 
So if you have so many projects, you'd Project ids can be searched by ssfind.rb.
If you have so many projects, you'd better make a shell script that calls ssconv 
based on the output of ssfind.rb.


Command line example for a single project conversion:

~~~
$ ./ssconv /Users/ringo/tmp/CCDK/backend/datadir ../../backend/datadir 4                                              (git)-[master]
datadir_in:'/Users/ringo/tmp/CCDK/backend/datadir' datadir_out:'../../backend/datadir' project_id:4
readFileOffset: can't open file:'../../backend/datadir/01/18/field_data_4_2048_2048_48' err:'No such file or directory'
................chunk y:0 total:240432
................chunk y:1 total:488287
................chunk y:2 total:740335
................chunk y:3 total:991553
................chunk y:4 total:1244885
................chunk y:5 total:1495762
................chunk y:6 total:1744010
................chunk y:7 total:1992167
................chunk y:8 total:2237683
(snipped)
................chunk y:125 total:31072434
................chunk y:126 total:31316973
................chunk y:127 total:31557035
~~~

Please ignore "readFileOffset: can't open .." error message, it just saying 
new format file is not found and it looks OK to write to the path.


### 4. Start backend server and test

### 5. Remove old formatted files.

You can remove all old files by this command line:

~~~
$ find /Users/ringo/tmp/CCDK/backend/datadir -name '*_16_16' -exec ls -l {} \;
~~~

