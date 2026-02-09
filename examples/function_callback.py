"""
functions callback
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
    result_1 = helper_function(5)
    result_2 = another_helper(6, 7)
    final_result = result_1 + result_2
    print(f"Final Result: {final_result}")


# This is valid - helper code outside main
if __name__ == "__main__":
    main()