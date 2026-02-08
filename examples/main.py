"""
Valid AIthon Project - Single main.py with one main() function
"""

def helper_function(x):
    """Helper function - this is allowed"""
    return x * 2

def another_helper(a, b):
    """Another helper - also allowed"""
    return a + b

def main():
    """
    This is the main entry point.
    Only ONE main() function is allowed per project.
    """
    result = helper_function(21)
    print(f"Result: {result}")
    return result

# This is valid - helper code outside main
if __name__ == "__main__":
    main()