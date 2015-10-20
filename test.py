import pysproto
from sys import getrefcount
from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode, sproto_pack, sproto_unpack, sproto_protocol
from binascii import b2a_hex, a2b_hex

with open("person.pb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
st = sproto_type(sp, "Person")
result = sproto_encode(st, {
    "name": "crystal",
    "id":1001,
    "email":"crystal@example.com",
    "phone":[
        {
            "type" : 1,
            "number": "10086",
        },
        {
            "type" : 2,
            "number":"10010",
        },
    ],
    })
print "result length:", len(result)
print b2a_hex(result)
print "-------------------------"
print sproto_decode(st, result)
decode_ret = sproto_decode(st, result)
refcount = getrefcount(decode_ret["name"]) - 1#extra 1 for used in temp args
assert(refcount==1)
refcount = getrefcount(decode_ret) - 1#extra 1 for used in temp args
assert(refcount==1)
refcount = getrefcount(decode_ret["phone"]) - 1#extra 1 for used in temp args
assert(refcount==1)
refcount = getrefcount(decode_ret["id"]) - 1#extra 1 for used in temp args
assert(refcount==1)
print "========================="
pack_result = sproto_pack(result)
print len(pack_result)
print b2a_hex(pack_result)
print "-------------------------"
unpack_result = sproto_unpack(pack_result)
print b2a_hex(unpack_result)
print "========================="
print "test exception catch function"
try:
    tmp = sproto_encode(st, {
        "name":"t",
        "id":"fake_id",
    })
except pysproto.error as e:
    print "catch encode error:\n", e
print "-------------------------"
with open("protocol.spb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
proto = sproto_protocol(sp, "foobar")
assert(getrefcount(proto[1])-1 == 1)
print "refcount seems good"
print "========================="
with open("testall.spb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
st = sproto_type(sp, "foobar")
msg = sproto_encode(st, {
    "a" : "hello",
    "b" : 1000000,
    "c" : True,
    "d" : {
        "world":{ 
                "a" : "world", 
                #skip b
                "c" : -1,
            },
        "two":{
                "a" : "two",
                "b" : True,
            },
        "":{
                "a" : "",
                "b" : False,
                "c" : 1,
        },
    },
    "e" : ["ABC", "", "def"],
    "f" : [-3, -2, -1, 0, 1, 2],
    "g" : [True, False, True],
    "h" : [
            {"b" : 100},
            {},
            {"b" : -100, "c" : False},
            {"b" : 0, "e" : ["test"]},
        ],
    }
    )
import pprint
pprint.pprint(sproto_decode(st, msg))
print "====================="
msg = sproto_encode(st, {
    "a" : "hello"*100000,
    })
assert(len(sproto_decode(st, msg)["a"]) == len("hello") * 100000)
