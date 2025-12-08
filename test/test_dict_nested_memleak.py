#!/usr/bin/env python3
"""
Test for dictionary nested reference count bug causing memory leak
Reference: https://github.com/spin6lock/python-sproto/pull/39

The bug: In decode() function, when handling SPROTO_TSTRUCT with map (mainindex >= 0),
sub.table is created with PyDict_New() (refcount=1), then added to parent dict with
PyDict_SetItem() (refcount becomes 2), but the original reference is never DECREF'd,
causing a memory leak.
"""
import pysproto
from pysproto import sproto_create, sproto_type, sproto_encode, sproto_decode, sproto_protocol
from sys import getrefcount
import unittest
import gc


class TestDictNestedMemLeak(unittest.TestCase):
    def setUp(self):
        """Setup test data"""
        with open("protocol.spb", "rb") as fh:
            content = fh.read()
        self.sp = sproto_create(content)
        _, self.req, _ = sproto_protocol(self.sp, "synheroinfos")
        
    def test_nested_dict_refcount(self):
        """Test nested dictionary reference count - detects memory leak"""
        source = {
            "herolist": {
                1: {
                    "id": 1,
                    "lv": 2,
                    "cfgid": 3,
                },
                2: {
                    "id": 2,
                    "lv": 3,
                    "cfgid": 4,
                },
            }
        }
        
        # Encode
        msg = sproto_encode(self.req, source)
        
        # Decode
        dest, r = sproto_decode(self.req, msg)
        
        # Check reference counts
        # Main dict should have refcount 1 (excluding getrefcount's own reference)
        main_refcount = getrefcount(dest) - 1
        print(f"\nMain dict refcount: {main_refcount} (expected: 1)")
        self.assertEqual(main_refcount, 1, 
                        f"Main dict refcount abnormal: {main_refcount}")
        
        # Check nested dict reference count
        # Get refcount directly from dict without creating intermediate variable
        # getrefcount() itself adds 1, so we subtract 1
        herolist_refcount = getrefcount(dest["herolist"]) - 1
        print(f"herolist dict refcount: {herolist_refcount} (expected: 1)")
        
        # Correct refcount should be 1 (only in parent dict, excluding getrefcount)
        # If != 1, the original reference from creation was not released (memory leak)
        if herolist_refcount != 1:
            print(f"??  MEMORY LEAK DETECTED: herolist dict refcount is {herolist_refcount}, should be 1")
            print("   Issue: sub.table is not Py_DECREF'd after PyDict_SetItem")
            print("   Location: python_sproto.c:285 in decode() function")
        self.assertEqual(herolist_refcount, 1,
                        f"herolist dict refcount abnormal: {herolist_refcount}")
        
        # Check each nested value dict
        # Use keys() to avoid items() iterator holding references
        herolist = dest["herolist"]
        for key in herolist:
            # Get refcount directly by key access, avoiding items() iterator reference
            # This way we only have: herolist dict reference + getrefcount temp reference
            hero_refcount = getrefcount(herolist[key]) - 1
            print(f"  hero dict (id={key}) refcount: {hero_refcount} (expected: 1)")
            if hero_refcount != 1:
                print(f"  ??  MEMORY LEAK: hero dict (id={key}) refcount is {hero_refcount}, should be 1")
            self.assertEqual(hero_refcount, 1,
                            f"hero dict (id={key}) refcount abnormal: {hero_refcount}")
        
        # Verify decode result is correct
        self.assertEqual(dest, source)
    
    def test_deeply_nested_dict(self):
        """Test deeply nested dictionary"""
        # Use nested structure from testall.sproto
        with open("testall.spb", "rb") as fh:
            content = fh.read()
        sp = sproto_create(content)
        st = sproto_type(sp, "foobar")
        
        source = {
            "d": {
                b"world": {
                    "a": b"world",
                    "c": -1,
                },
                b"two": {
                    "a": b"two",
                    "b": True,
                },
            },
        }
        
        msg = sproto_encode(st, source)
        
        # Multiple decodes
        leak_detected = False
        for i in range(50):
            dest, r = sproto_decode(st, msg)
            self.assertEqual(dest, source)
            
            # Check refcount
            # Get refcount directly without creating intermediate variable
            d_refcount = getrefcount(dest["d"]) - 1
            if i % 10 == 0:
                print(f"Iteration {i}: d dict refcount: {d_refcount} (expected: 1)")
                if d_refcount != 1:
                    print(f"??  Iteration {i}: MEMORY LEAK in d dict (refcount: {d_refcount}, should be 1)")
                    leak_detected = True
            
            d_dict = dest["d"]  # Create variable for iteration
            for key in d_dict:
                # Get refcount directly without intermediate variable
                nested_refcount = getrefcount(d_dict[key]) - 1
                if i % 10 == 0 and nested_refcount != 1:
                    print(f"??  Iteration {i}: MEMORY LEAK in nested dict (refcount: {nested_refcount}, should be 1)")
                    leak_detected = True
        
        if leak_detected:
            print("\n??  MEMORY LEAK CONFIRMED in deeply nested dict test")


if __name__ == "__main__":
    unittest.main()
