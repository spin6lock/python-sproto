import pysproto
from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode
import gc

print("***************************")
with open("person.spb", "rb") as fh:
    content = fh.read()
def get_st(content, tag):
    sp = sproto_create(content)
    st = sproto_type(sp, tag)
    return st
gc.collect()
st = get_st(content, "Person")
result = sproto_encode(st, {
    "name": b"crystal",
    "id":1001,
    "email":b"crystal@example.com",
    "phone":[
        {
            "type" : 1,
            "number": b"10086",
        },
        {
            "type" : 2,
            "number": b"10010",
        },
    ],
    })
print(sproto_decode(st, result))
