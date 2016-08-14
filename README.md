oradump
=======

A fast multi-threaded bulk downloader for Oracle queries written in C++11 style and designed to batch process numerous queries concurrently

###Why?
You can use sqlplus to do bulk downloads as csv files and spin multiple sessions.  

But you'll have to do extra work to get the column metadata.
You'll need that metadata when you need to create a destination table in another rdbms or a NoSQL like Cassandra.

Use oradump instead.

Comes with logging too.

###Usage
```
# oradump always expects ~ONE~ SQL statment per file

# Process one file
oradump --sid test --user scott -pass tiger --file /home/foo/some.sql

# Process entire directory
oradump --sid test --user scott -pass tiger --dir /home/foo/lotta-sql-path/
```
###Gotchas
Make sure you set __TNS_ADMIN__ in your environment and that the admin folder contains valid tnsnames.ora and sqlnet.ora or your app will crash

###Dependencies
This project could not be possible without the following fabulous libraries
* [ocilib](http://orclib.sourceforge.net/) Oracle OCI wrapper
* [spdlog](https://github.com/gabime/spdlog) Extremely fast logger
* [boost](http://sourceforge.net/projects/boost/files/boost-binaries/) Well, you know

###Install
Compiled using VS2013, will definitely work with g++ with -std=c++11 flag.