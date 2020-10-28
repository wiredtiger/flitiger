# bson-indexer
A usage of the WiredTiger library that splits BSON documents and stores them fully indexed.

A link to the (internal to MongoDB) document accompanying this repo:
https://docs.google.com/document/d/1Exe9mpautvl-dwiIQZu6l7HPkFG7i1nG0a_-Cci8SDY/edit#heading=h.rvldxpy3jdvd

# Installation

You need a locally built WiredTiger from source.
https://github.com/wiredtiger/wiredtiger/tree/develop

You need to set an environment variable telling this project where to find WiredTiger:
`export WT_HOME=~/work/wiredtiger`

There is a dependency on cpprestsdk (https://github.com/microsoft/cpprestsdk), which can be installed via:

`sudo apt-get install libcpprest-dev`

If you aren't using Ubuntu, then the above link has instructions for other platforms.


Once the dependencies are met: `cd src && make`


# Usage

To use the library, run the generated binary.

If running in server mode, you can send requests using curl in the following form:

`~/work/bson-indexer$ curl http://127.0.0.1:8099/test --data-binary @raw_data/rockbench_1row.json -H 'Content-Type: application/json'`

## Using Rockbench to populate FliTiger

Clone the Rockbench repository.
Apply the diff at <root>/raw_data/rbench.diff to the repository.
Build the generator.
Run against a flitiger running in server mode.

Starting up a flitiger in code:
```
$ cd ~/work/bson-indexer/src
$ make
$ ./wiredindex -S
```

Starting up a generator in code:
```
$ cd ~/work
$ git clone git@github.com:rockset/rockbench.git
$ cd rockbench
$ patch -p1 < ../bson-index/raw_data/rbench.diff
$ cd generator
$ go build
$ FLITIGER_URL="http://127.0.0.1:8099/" BATCH_SIZE=2 WPS=10 DESTINATION=flitiger ./generator
```

