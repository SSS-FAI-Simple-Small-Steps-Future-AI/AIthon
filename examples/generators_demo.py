"""
Example demonstrating generators and iterators
"""

def fibonacci_generator(n):
    """Generate Fibonacci numbers up to n"""
    a, b = 0, 1
    count = 0
    while count < n:
        yield a
        a, b = b, a + b
        count += 1

def range_generator(start, stop, step=1):
    """Custom range generator"""
    current = start
    while current < stop:
        yield current
        current += step

def squares_generator(n):
    """Generate squares of numbers"""
    for i in range(n):
        yield i * i

def filter_evens(numbers):
    """Generator that filters even numbers"""
    for num in numbers:
        if num % 2 == 0:
            yield num

def generator_pipeline():
    """Demonstrate generator composition"""
    # Create a pipeline: range -> squares -> filter evens
    numbers = range_generator(0, 20)
    squares = (x * x for x in numbers)
    evens = filter_evens(squares)

    return list(evens)

def test_generators():
    print("Fibonacci sequence:")
    for num in fibonacci_generator(10):
        print(num, end=" ")
    print()

    print("\nCustom range:")
    for num in range_generator(0, 20, 3):
        print(num, end=" ")
    print()

    print("\nSquares:")
    for num in squares_generator(5):
        print(num, end=" ")
    print()

    print("\nPipeline result:")
    result = generator_pipeline()
    print(result)

    # Generator expressions
    print("\nGenerator expression:")
    doubled = (x * 2 for x in range(5))
    for num in doubled:
        print(num, end=" ")
    print()

if __name__ == "__main__":
    test_generators()