#include "runtime/pyobject.h"
#include "runtime/exceptions.h"
#include <iostream>
#include <cassert>

using namespace pyvm::runtime;

void test_integers() {
    std::cout << "Testing integers...\n";
    
    auto a = make_int(10);
    auto b = make_int(20);
    
    auto sum = a->add(b);
    assert(std::static_pointer_cast<PyInt>(sum)->value() == 30);
    
    auto diff = b->sub(a);
    assert(std::static_pointer_cast<PyInt>(diff)->value() == 10);
    
    auto prod = a->mul(b);
    assert(std::static_pointer_cast<PyInt>(prod)->value() == 200);
    
    auto quot = b->div(a);
    assert(std::static_pointer_cast<PyFloat>(quot)->value() == 2.0);
    
    auto mod = b->mod(make_int(7));
    assert(std::static_pointer_cast<PyInt>(mod)->value() == 6);
    
    std::cout << "✓ Integer tests passed\n";
}

void test_strings() {
    std::cout << "Testing strings...\n";
    
    auto hello = make_string("Hello");
    auto world = make_string(" World");
    
    auto greeting = hello->add(world);
    assert(std::static_pointer_cast<PyString>(greeting)->value() == "Hello World");
    
    auto repeated = hello->mul(make_int(3));
    assert(std::static_pointer_cast<PyString>(repeated)->value() == "HelloHelloHello");
    
    assert(hello->len() == 5);
    
    auto first_char = hello->getitem(make_int(0));
    assert(std::static_pointer_cast<PyString>(first_char)->value() == "H");
    
    std::cout << "✓ String tests passed\n";
}

void test_lists() {
    std::cout << "Testing lists...\n";
    
    auto list = make_list();
    auto py_list = std::static_pointer_cast<PyList>(list);
    
    py_list->append(make_int(1));
    py_list->append(make_int(2));
    py_list->append(make_int(3));
    
    assert(py_list->len() == 3);
    
    auto item = py_list->getitem(make_int(1));
    assert(std::static_pointer_cast<PyInt>(item)->value() == 2);
    
    py_list->setitem(make_int(1), make_int(10));
    item = py_list->getitem(make_int(1));
    assert(std::static_pointer_cast<PyInt>(item)->value() == 10);
    
    // List concatenation
    auto list2 = make_list();
    std::static_pointer_cast<PyList>(list2)->append(make_int(4));
    auto combined = list->add(list2);
    assert(std::static_pointer_cast<PyList>(combined)->len() == 4);
    
    // List multiplication
    auto repeated = list->mul(make_int(2));
    assert(std::static_pointer_cast<PyList>(repeated)->len() == 6);
    
    std::cout << "✓ List tests passed\n";
}

void test_dicts() {
    std::cout << "Testing dictionaries...\n";
    
    auto dict = make_dict();
    auto py_dict = std::static_pointer_cast<PyDict>(dict);
    
    py_dict->setitem(make_string("name"), make_string("Alice"));
    py_dict->setitem(make_string("age"), make_int(30));
    py_dict->setitem(make_string("score"), make_float(95.5));
    
    assert(py_dict->len() == 3);
    
    auto name = py_dict->getitem(make_string("name"));
    assert(std::static_pointer_cast<PyString>(name)->value() == "Alice");
    
    auto age = py_dict->getitem(make_string("age"));
    assert(std::static_pointer_cast<PyInt>(age)->value() == 30);
    
    std::cout << "✓ Dictionary tests passed\n";
}

void test_exceptions() {
    std::cout << "Testing exceptions...\n";
    
    bool caught = false;
    try {
        auto list = make_list();
        list->getitem(make_int(10));  // Should throw IndexError
    } catch (const std::runtime_error& e) {
        caught = true;
        assert(std::string(e.what()).find("out of range") != std::string::npos);
    }
    assert(caught);
    
    caught = false;
    try {
        auto a = make_int(10);
        auto b = make_int(0);
        a->div(b);  // Should throw division by zero
    } catch (const std::runtime_error& e) {
        caught = true;
        assert(std::string(e.what()).find("zero") != std::string::npos);
    }
    assert(caught);
    
    std::cout << "✓ Exception tests passed\n";
}

void test_classes() {
    std::cout << "Testing classes...\n";
    
    // Create a simple class
    auto my_class = std::make_shared<PyClass>("MyClass");
    
    // Add a method
    auto method = std::make_shared<PyFunction>("greet", 
        [](const std::vector<std::shared_ptr<PyObject>>& args) {
            return make_string("Hello from MyClass!");
        }
    );
    my_class->add_method("greet", method);
    
    // Create instance
    auto instance = my_class->call({});
    auto py_instance = std::static_pointer_cast<PyInstance>(instance);
    
    // Set instance attribute
    py_instance->set_attr("name", make_string("Test"));
    auto name = py_instance->get_attr("name");
    assert(std::static_pointer_cast<PyString>(name)->value() == "Test");
    
    std::cout << "✓ Class tests passed\n";
}

void test_comparisons() {
    std::cout << "Testing comparisons...\n";
    
    auto a = make_int(10);
    auto b = make_int(20);
    
    auto lt = a->lt(b);
    assert(std::static_pointer_cast<PyBool>(lt)->value() == true);
    
    auto gt = b->gt(a);
    assert(std::static_pointer_cast<PyBool>(gt)->value() == true);
    
    auto eq = a->eq(make_int(10));
    assert(std::static_pointer_cast<PyBool>(eq)->value() == true);
    
    auto ne = a->ne(b);
    assert(std::static_pointer_cast<PyBool>(ne)->value() == true);
    
    std::cout << "✓ Comparison tests passed\n";
}

void test_type_conversions() {
    std::cout << "Testing type conversions...\n";
    
    // Int + Float = Float
    auto int_val = make_int(10);
    auto float_val = make_float(3.5);
    
    auto result = int_val->add(float_val);
    assert(result->type() == PyType::FLOAT);
    assert(std::static_pointer_cast<PyFloat>(result)->value() == 13.5);
    
    // String * Int = String
    auto str_val = make_string("abc");
    auto repeated = str_val->mul(make_int(3));
    assert(repeated->type() == PyType::STRING);
    assert(std::static_pointer_cast<PyString>(repeated)->value() == "abcabcabc");
    
    std::cout << "✓ Type conversion tests passed\n";
}

void test_truthiness() {
    std::cout << "Testing truthiness...\n";
    
    assert(make_int(0)->is_true() == false);
    assert(make_int(1)->is_true() == true);
    assert(make_string("")->is_true() == false);
    assert(make_string("hello")->is_true() == true);
    assert(make_list()->is_true() == false);
    
    auto list = make_list();
    std::static_pointer_cast<PyList>(list)->append(make_int(1));
    assert(list->is_true() == true);
    
    std::cout << "✓ Truthiness tests passed\n";
}

int main() {
    std::cout << "\n=== Running Python Object System Tests ===\n\n";
    
    try {
        test_integers();
        test_strings();
        test_lists();
        test_dicts();
        test_exceptions();
        test_classes();
        test_comparisons();
        test_type_conversions();
        test_truthiness();
        
        std::cout << "\n=== All Tests Passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}