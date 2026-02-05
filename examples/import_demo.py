"""
Example demonstrating import system
"""

# Standard library imports
import sys
import os
import math
from collections import defaultdict, Counter
from itertools import chain, combinations

# Module imports
from mymodule import helper_function, MyClass
from package.submodule import another_function

def test_stdlib():
    """Test standard library functionality"""
    print("Python version:", sys.version)
    print("Platform:", sys.platform)

    # Math operations
    print("Pi:", math.pi)
    print("Square root of 16:", math.sqrt(16))
    print("Sine of 45 degrees:", math.sin(math.radians(45)))

    # Collections
    counter = Counter([1, 2, 2, 3, 3, 3])
    print("Counter:", counter)

    default_dict = defaultdict(int)
    default_dict['key1'] += 1
    print("DefaultDict:", dict(default_dict))

    # Itertools
    items = [1, 2, 3]
    print("Combinations:", list(combinations(items, 2)))

    # OS operations
    print("Current directory:", os.getcwd())
    print("Environment variables:", os.environ.get('HOME'))

def test_custom_modules():
    """Test custom module imports"""
    result = helper_function(42)
    print("Helper result:", result)

    obj = MyClass("test")
    obj.do_something()

    data = another_function()
    print("Data from submodule:", data)

def test_dynamic_import():
    """Test dynamic imports"""
    module_name = "json"
    module = __import__(module_name)

    data = {"key": "value", "number": 42}
    json_string = module.dumps(data)
    print("JSON:", json_string)

    parsed = module.loads(json_string)
    print("Parsed:", parsed)

if __name__ == "__main__":
    test_stdlib()
    # test_custom_modules()  # Requires mymodule.py
    # test_dynamic_import()