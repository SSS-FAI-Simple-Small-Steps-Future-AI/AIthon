add_executable(test_scheduler test_scheduler.cpp)
target_link_libraries(test_scheduler pyvm_runtime pthread)

add_executable(test_actors test_actors.cpp)
target_link_libraries(test_actors pyvm_runtime pthread)

add_executable(test_pyobject test_pyobject.cpp)
target_link_libraries(test_pyobject pyvm_runtime pthread)

add_executable(test_validator test_validator.cpp ../src/validator/project_validator.cpp)
target_link_libraries(test_validator pthread)

add_test(NAME SchedulerTest COMMAND test_scheduler)
add_test(NAME ActorTest COMMAND test_actors)
add_test(NAME PyObjectTest COMMAND test_pyobject)
add_test(NAME ValidatorTest COMMAND test_validator)