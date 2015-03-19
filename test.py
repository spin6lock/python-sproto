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
with open("int_arr.spb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
st = sproto_type(sp, "Phone")
msg = sproto_encode(st, {
        "phonenumber": [
            1,2,3,4,5
            ]
    })
print sproto_decode(st, msg)
