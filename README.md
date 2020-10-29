# FLITiger
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

`~/work/flitiger$ curl http://127.0.0.1:8099/test --data-binary @raw_data/rockbench_1row.json -H 'Content-Type: application/json'`

## Using Rockbench to populate FLITiger

Clone the Rockbench repository.
Apply the diff at <root>/raw_data/rbench.diff to the repository.
Build the generator.
Run against a flitiger running in server mode.

Starting up a flitiger in code:
```
$ cd ~/work/flitiger/src
$ make
$ ./flitiger -S
```

Starting up a generator in code:
```
$ cd ~/work
$ git clone git@github.com:rockset/rockbench.git
$ cd rockbench
$ patch -p1 < ../bson-index/raw_data/rbench.diff
$ cd generator
$ go get ./...
$ go build
$ FLITIGER_URL="http://127.0.0.1:8099/" BATCH_SIZE=2 WPS=10 DESTINATION=flitiger ./generator
```

## Writing data to S3

Change the table configuration in the application to write level 2 and above chunks to an `s3` directory:

```
diff --git a/src/main.cpp b/src/main.cpp
index 5a41481..73e4b41 100644
--- a/src/main.cpp
+++ b/src/main.cpp
@@ -147,12 +147,12 @@ int main(int argc, char **argv)
     assert(conn);
     assert(session);

-    std::string config = "type=lsm,key_format=QS,value_format=Hu";
+    std::string config = "key_format=QS,value_format=Hu,type=lsm,lsm=(merge_custom=(prefix=\"file:s3/\",suffix=\".lsm\",start_generation=2))";
     if ((ret = wt::create_table(session, rtbl, config)) != 0) {
         std::cout << wt::get_error_message(ret) << '\n';
         return ret;
     }
-    config = "type=lsm,key_format=SQ,value_format=Hu";
+    config = "key_format=SQ,value_format=Hu,type=lsm,lsm=(merge_custom=(prefix=\"file:s3/\",suffix=\".lsm\",start_generation=2))";
     if ((ret = wt::create_table(session, ctbl, config)) != 0) {
         std::cout << wt::get_error_message(ret) << '\n';
         return ret;
```

Rebuild: `cd src && make`.

Install s3fs:

```
$ sudo amazon-linux-extras install epel
$ sudo yum install s3fs-fuse
```

Create an S3 bucket at https://s3.console.aws.amazon.com/s3/home?region=ap-southeast-2

Store your AWS credentials:

```
$ echo ACCESS_KEY_ID:SECRET_ACCESS_KEY > ${HOME}/.passwd-s3fs
$ chmod 600 ${HOME}/.passwd-s3fs
```

Mount the bucket as a filesystem called `s3` inside the data directory:

```
$ S3_REGION=ap-southeast-2
$ s3fs flitiger wt_test/s3 -o dbglevel=info -o endpoint=${S3_REGION} -o passwd_file=${HOME}/.passwd-s3fs -o url=https://s3-${S3_REGION}.amazonaws.com/
```

Run the workload as before: once enough data is inserted, merges will be triggered that create level 2 chunks (after around 60 chunks in each LSM tree).
