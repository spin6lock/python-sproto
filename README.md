First, you need a sproto compiler to compile sproto
description to binary description. You can do it offline with https://github.com/lvzixun/sproto_dump. 
Then you can use sproto decode and encode interface.

* sproto_create:

    create a sproto object from binary

* sproto_type:

    query the entity for pack

* sproto_encode:

    encode data in dict with entity description

* sproto_decode:

    decode data in message with entity description

Compile:

    python setup.py build

    Then copy the example.so from build directory to python script directory.
