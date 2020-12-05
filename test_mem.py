import pysproto
from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode
import unittest

class TestPySproto(unittest.TestCase):
    def test_complex_encode_decode(self):
        with open("testall.spb", "rb") as fh:
            content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "foobar")
        source = {
            "g" : [True, False, True, False],
        }
        msg = sproto_encode(st, source)
        for i in range(1000):
            dest, r = sproto_decode(st, msg)
        self.assertEqual(dest, source)
        self.assertEqual(r, len(msg))


if __name__ == "__main__":
    unittest.main()
