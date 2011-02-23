nginx-mongodb-rest
=========
An nginx module which works as a mongodb readonly rest client.

Dependencies
============
**nginx-mongodb-rest** requires the mongo-c-driver which is a submodule of this
repository. To checkout the submodule run

    $ git submodule init
    $ git submodule update


Installation
============
Installing Nginx modules requires rebuilding Nginx from source:

- Grab the [Nginx source](http://nginx.net/) and unpack it.
- Clone this repository somewhere on your machine `git clone git://github.com/minhajuddin/nginx-mongodb-rest.git`.
- Check out the required submodule, as described above.
- Change to the directory containing the Nginx source.
- Now build:

        $ ./configure --add-module=/path/to/nginx-gridfs/source/
        $ make
        $ make install

Configuration
=============

    location / {
        mongodb_rest "database_name" "collection_name";
    }

    #example config
    location / {
        mongodb_rest "khalid_dev" "products";
    }

The above example configuration exposes the *products* collection in the
*khalid_dev* database. If the server is running on your `localhost` the command
`curl -i http://localhost/12` will give you the following output:

    HTTP/1.1 200 OK
    Server: nginx/0.8.54
    Date: Wed, 23 Feb 2011 23:23:11 GMT
    Content-Type: application/json
    Transfer-Encoding: chunked
    Connection: keep-alive

    {"_id" : "12" , "name": "funky widgets"}

Known Issues / TODO / Things You Should Hack On
===============================================
- allow passing *collection name* through the url
- allow *ids* of different types e.g. *bson_oid*, *int* ..
- allow *scriptable* collection names

Authors
=======
Khaja Minhajuddin (minhajuddin&#64;cosmicvent.com), Nagaraju BVS (nagaraju&#64;cosmicvent.com)

Credits
=======
- A lot of the code was re-used/copied from [nginx-gridfs](https://github.com/mdirolf/nginx-gridfs.git)

License
=======
**nginx-mongodb-rest** is licensed under the Apache License, Version 2.0. See *LICENSE* for more details.
