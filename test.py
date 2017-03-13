import pysproto
from sys import getrefcount
from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode, sproto_pack, sproto_unpack, sproto_protocol, sproto_dump, sproto_name
from binascii import b2a_hex, a2b_hex
import unittest

class TestPySproto(unittest.TestCase):
    def get_st_sp(self):
        with open("person.spb", "r") as fh:
            content = fh.read()
            sp = sproto_create(content)
            st = sproto_type(sp, "Person")
        return st, sp

    def test_basic_encode_decode(self):
        st, sp = self.get_st_sp()
        source = {
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
        }
        result = sproto_encode(st, source)
        expected = a2b_hex("04000000d40700000000070000006372797374616c130000006372797374616c406578616d706c652e636f6d260000000f0000000200000004000500000031303038360f000000020000000600050000003130303130")
        self.assertEqual(result, expected)
        #print b2a_hex(result)
        dest, r = sproto_decode(st, result)
        self.assertEqual(source, dest)
        self.assertEqual(r, len(expected))

    def test_refcount(self):
        st, sp = self.get_st_sp()
        result = a2b_hex("04000000d40700000000070000006372797374616c130000006372797374616c406578616d706c652e636f6d260000000f0000000200000004000500000031303038360f000000020000000600050000003130303130")
        decode_ret, r = sproto_decode(st, result)
        self.assertEqual(getrefcount(decode_ret["name"]) - 1, 1)#extra 1 for used in temp args
        self.assertEqual(getrefcount(decode_ret["phone"]) - 1, 1)#extra 1 for used in temp args
        self.assertEqual(getrefcount(decode_ret["id"]) - 1, 1)#extra 1 for used in temp args
        self.assertEqual(getrefcount(decode_ret) - 1, 1)#extra 1 for used in temp args

    def test_sproto_pack(self):
        result = a2b_hex("04000000d40700000000070000006372797374616c130000006372797374616c406578616d706c652e636f6d260000000f0000000200000004000500000031303038360f000000020000000600050000003130303130")
        pack_result = sproto_pack(result)
        #print len(pack_result)
        #print b2a_hex(pack_result)
        expected = a2b_hex("3104d407c40763723f797374616c13fe6372797374616cff00406578616d706c651f2e636f6d26110f02c5040531308f3038360f022806053e3130303130")
        self.assertEqual(expected, pack_result)

    """
    https://github.com/cloudwu/sproto/issues/43 
    """
    def test_sproto_pack_bug(self):
        result = a2b_hex("e7"*30)
        pack_result = sproto_pack(result)
        expected = a2b_hex("ff03"+"e7"*30+"0000")
        self.assertEqual(pack_result, expected)

    def test_sproto_unpack(self):
        result = a2b_hex("3104d407c40763723f797374616c13fe6372797374616cff00406578616d706c651f2e636f6d26110f02c5040531308f3038360f022806053e3130303130")
        unpack_result = sproto_unpack(result)
        #print b2a_hex(unpack_result)
        expected = a2b_hex("04000000d40700000000070000006372797374616c130000006372797374616c406578616d706c652e636f6d260000000f0000000200000004000500000031303038360f0000000200000006000500000031303031300000")
        self.assertEqual(expected, unpack_result)

    """
    https://github.com/spin6lock/python-sproto/issues/6
    """
    def test_sproto_unpack_bug(self):
        data = a2b_hex("123456789ABCDEF" * 256)
        pack_result = sproto_pack(data)
        unpack_data = sproto_unpack(pack_result)
        self.assertEqual(data, unpack_data)

    def test_exception_catch(self):
        st, sp = self.get_st_sp()
        with self.assertRaises(pysproto.error) as se:
            tmp = sproto_encode(st, {
                "name":"t",
                "id":"fake_id",
            })
        self.assertEqual(se.exception.message, "type mismatch, tag:id, expected int or long")

    def test_sproto_protocal_refcount(self):
        with open("protocol.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        proto = sproto_protocol(sp, "foobar")
        self.assertEqual(getrefcount(proto[1])-1, 1)

    def test_sproto_protocal(self):
        with open("protocol.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        tag, req, resp = sproto_protocol(sp, "foo")
        self.assertEqual(req, None)
        self.assertNotEqual(resp, None)

    def test_complex_encode_decode(self):
        with open("testall.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "foobar")
        source = {
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
        msg = sproto_encode(st, source)
        dest, r = sproto_decode(st, msg)
        #import pprint
        #pprint.pprint(sproto_decode(st, msg))
        self.assertEqual(dest, source)
        self.assertEqual(r, len(msg))

    def test_long_string_encode(self):
        with open("testall.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "foobar")
        origin_str = "hello"*100000
        msg = sproto_encode(st, {
            "a" : origin_str,
            })
        decode_result, r = sproto_decode(st, msg)
        self.assertEqual(decode_result["a"], origin_str)

    def test_sproto_dump(self):
        with open("testall.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        sproto_dump(sp)

    def test_sproto_name(self):
        st, _ = self.get_st_sp()
        print sproto_name(st)

    #https://github.com/spin6lock/python-sproto/issues/7
    def test_long_long_cutoff(self):
        with open("testall.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "foobar")
        b = pow(2, 63) - 1
        msg = sproto_encode(st, {
            "b" : b,
            })
        decode_result, r = sproto_decode(st, msg)
        self.assertEqual(decode_result["b"], b)

    def test_empty_table(self):
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
        dest, r = sproto_decode(st, msg)
        #import pprint
        #pprint.pprint(sproto_decode(st, msg))
        self.assertEqual(dest, source)
        self.assertEqual(r, len(msg))

    def test_unicode_string(self):
        with open("testall.spb", "r") as fh:
            content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "Person")
        origin_str = u'hello'
        msg = sproto_encode(st, {
            'name' : origin_str,
            })
        decode_result, r = sproto_decode(st, msg)
        self.assertEqual(decode_result['name'], origin_str)

if __name__ == "__main__":
    unittest.main()
