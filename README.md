[![Build Status](https://travis-ci.org/spin6lock/python-sproto.svg?branch=master)](https://travis-ci.org/spin6lock/python-sproto)

What's new:
===========
20210904
* upload to pypi finally, you can install Python3 version by `pip3 install pysproto`

20170313
* Add fixed-point number support
* Add unicode string support, any field with declare string must be unicode, and it will send out with utf-8 encoded. If you encoded string yourself, then use binary instead

IMPORTANT:
==========
The sproto_decode api has changed recently, so you may need to change the code to accept ret value. Sorry for the inconvenient.

First, you need a sproto compiler to compile sproto
description to binary description. You can do it offline with [sproto_dump](https://github.com/lvzixun/sproto_dump).
Then you can use sproto decode and encode interface.

Example:
----------

```python
  with open('your.spb', 'r') as fh:
    content = fh.read()
  sproto_obj = SprotoRpc(content, content, base_package) #base_package is your package struct name
  p = sproto_obj.request('client.hello', {'foo':'bar'})
  sock.send(p)
```

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
