"""
Example demonstrating classes, inheritance, and exception handling
"""

class Animal:
    def __init__(self, name, age):
        self.name = name
        self.age = age

    def speak(self):
        return "Some sound"

    def get_info(self):
        return f"{self.name} is {self.age} years old"

class Dog(Animal):
    def __init__(self, name, age, breed):
        super().__init__(name, age)
        self.breed = breed

    def speak(self):
        return "Woof! Woof!"

    def fetch(self):
        return f"{self.name} is fetching the ball!"

class Cat(Animal):
    def speak(self):
        return "Meow!"

def test_animals():
    try:
        # Create instances
        dog = Dog("Buddy", 3, "Golden Retriever")
        cat = Cat("Whiskers", 5)

        # Test methods
        print(dog.get_info())  # Buddy is 3 years old
        print(dog.speak())      # Woof! Woof!
        print(dog.fetch())      # Buddy is fetching the ball!

        print(cat.get_info())   # Whiskers is 5 years old
        print(cat.speak())      # Meow!

        # Test exceptions
        animals = [dog, cat]
        for animal in animals:
            print(f"{animal.name} says: {animal.speak()}")

        # Index error
        print(animals[10])  # Raises IndexError

    except IndexError as e:
        print(f"Caught error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        print("Cleanup complete")

if __name__ == "__main__":
    test_animals()