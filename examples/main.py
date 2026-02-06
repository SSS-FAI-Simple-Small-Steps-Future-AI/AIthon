"""
Valid PyVM Project - Single main.py with one main() function
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
    print("Starting PyVM program")

    result = helper_function(21)
    print(f"Result: {result}")

    sum_result = another_helper(10, 20)
    print(f"Sum: {sum_result}")

    print("Program completed successfully")

# This is valid - helper code outside main
if __name__ == "__main__":
    main()