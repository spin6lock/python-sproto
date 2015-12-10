[![Build Status](https://travis-ci.org/spin6lock/python-sproto.svg?branch=master)](https://travis-ci.org/spin6lock/python-sproto)

First, you need a sproto compiler to compile sproto
description to binary description. You can do it offline with [sproto_dump](https://github.com/lvzixun/sproto_dump).
Then you can use sproto decode and encode interface.

Interface:
----------

* sproto_obj = sproto_create(your_sproto_description_in_spb)

    create a sproto object from binary

* sproto_type_obj = sproto_type(sproto_obj, entity_name)

    query the entity for pack

* binary_str = sproto_encode(sproto_type_obj, data_dict)

    encode data in dict with entity description

* data_dict = sproto_decode(sproto_type_obj, binary_str)

    decode data in message with entity description
    
* protocol_id_or_name, protocol_request, protocol_response = sproto_protocol(sproto_obj, name_or_protocol_id)

    query protocol tag id to protocol name
    query protocol name to protocol id
    return protocol_request, protocol_response

* packed_binary_string = sproto_pack(binary_str)

    sproto_pack will pack the sproto_encode msg to small size.
    
* original_binary_string = sproto_unpack(packed_binary_string)
 
    sproto_unpack restore the origin message. Note that there maybe more zero bytes at the end of the result, in order to align.

Error Handling:
---------------
all error is in pysproto.error

Compile:
--------

```
    python setup.py build
```
    
Then copy the example.so from build directory to python script directory.

Support:
-------
Python2.7 supported
Python2.6 is not supported currently
