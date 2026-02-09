"""
Valid AIthon Project - Single main.py with one main() function
"""


def main():
    """
    This is the main entry point.
    Only ONE main() function is allowed per project.
    """
    x = 4
    result = x + 1
    print(result)

# This is valid - helper code outside main
if __name__ == "__main__":
    main()