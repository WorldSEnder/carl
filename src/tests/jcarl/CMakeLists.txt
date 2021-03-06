find_package(Java REQUIRED COMPONENTS Runtime Development)
find_package(JNI REQUIRED)
include(UseJava)

set(JCARL_BINARY_DIR "${CMAKE_BINARY_DIR}/bindings/jcarl")

set(test_filenames "")
file(GLOB_RECURSE test_files "./*.java")
foreach(file ${test_files})
	get_filename_component(filename ${file} NAME_WE)
	IF (NOT (${filename} MATCHES "TestCarl")) 
		set(test_filenames ${test_filenames} ${filename})
	ENDIF ()
endforeach()


enable_testing()

#TODO: probably find location of junit.jar automatically
#compile the tests
ADD_CUSTOM_COMMAND(
OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/*.class
COMMAND ${Java_JAVAC_EXECUTABLE} -d ${CMAKE_CURRENT_BINARY_DIR} -cp ".:${CMAKE_CURRENT_SOURCE_DIR}:/usr/share/java/junit.jar"
${CMAKE_CURRENT_SOURCE_DIR}/*java
DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.java
WORKING_DIRECTORY ${JCARL_BINARY_DIR}
)


add_test(NAME jcarl COMMAND ${Java_JAVA_EXECUTABLE} -classpath .:/usr/share/java/junit.jar:/usr/share/java/hamcrest-core.jar:${JCARL_BINARY_DIR}/ -Djava.library.path=${JCARL_BINARY_DIR}/../ org.junit.runner.JUnitCore ${test_filenames}
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


ADD_CUSTOM_TARGET(jcarl_tests
ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.class)

ADD_DEPENDENCIES(jcarl_tests jcarl.jar jcarl)
