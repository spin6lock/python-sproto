import pysproto
from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode, sproto_pack, sproto_unpack, sproto_protocol

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
print ''.join(["%02x" %ord(x) for x in result])
print "-------------------------"
print sproto_decode(st, result)
print "========================="
pack_result = sproto_pack(result)
print len(pack_result)
print ''.join(["%02x" %ord(x) for x in pack_result])
print "-------------------------"
unpack_result = sproto_unpack(pack_result)
print ''.join(["%02x" %ord(x) for x in unpack_result])
print "========================="
try:
    tmp = sproto_encode(st, {
        "name":"t",
        "id":"fake_id",
    })
except pysproto.error:
    print "catch encode error"
print ""
print "-------------------------"
with open("protocol.spb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
print sproto_protocol(sp, "foobar")
print "========================="
with open("testall.spb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
st = sproto_type(sp, "foobar")
msg = sproto_encode(st, {
    "a" : "hello",
    "b" : 1000000,
    "c" : True,
    "d" : [
            { 
                "a" : "world", 
                #skip b
                "c" : -1,
            },
            {
                "a" : "two",
                "b" : True,
            },
            {
                "a" : "",
                "b" : False,
                "c" : 1,
            },
    ],
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
