# bson-indexer
A usage of the WiredTiger library that splits BSON documents and stores them fully indexed.


A link to the (internal to MongoDB) document accompanying this repo:
https://docs.google.com/document/d/1Exe9mpautvl-dwiIQZu6l7HPkFG7i1nG0a_-Cci8SDY/edit#heading=h.rvldxpy3jdvd

# Installation

You need a locally built WiredTiger from source.
https://github.com/wiredtiger/wiredtiger/tree/develop

You need to set an environment variable telling this project where to find WiredTiger:
export WT_HOME=~/work/wiredtiger

There is a dependency on cpprestsdk (https://github.com/microsoft/cpprestsdk), which can be installed via:

sudo apt-get install libcpprest-dev

If you aren't using Ubuntu, then the above link has instructions for other platforms.


Once the dependencies are met: cd src && make
