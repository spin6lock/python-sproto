from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode, sproto_pack, sproto_unpack, sproto_protocol, sproto_dump, sproto_name
from binascii import b2a_hex, a2b_hex

def get_st_sp():
    with open("person.pb", "r") as fh:
        content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "Person")
    return st, sp

def test_basic_encode_decode():
    with open("testall.spb", "r") as fh:
        content = fh.read()
    sp = sproto_create(content)
    st = sproto_type(sp, "foobar")
    source = {
        "d" : {
        },
        "e" : [],
        "f" : [],
        "g" : [],
        "h" : [
            ],
        }
    msg = sproto_encode(st, source)
    print "encode finish, =========================", len(msg)
    dest, r = sproto_decode(st, msg)
    #import pprint
    #pprint.pprint(sproto_decode(st, msg))
    print dest

test_basic_encode_decode()
