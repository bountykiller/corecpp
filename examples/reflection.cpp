#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <typeinfo>

#include <corecpp/meta/extensions.h>

int main(int argc, char** argv)
{
	std::cout << "std::vector<int> is iterable? " << corecpp::is_iterable<std::vector<int>>::value << std::endl;
	std::cout << "std::string is iterable? " << corecpp::is_iterable<std::string>::value << std::endl;
	std::cout << "std::optional<int> is iterable? " << corecpp::is_iterable<std::optional<int>>::value << std::endl;
	std::cout << "std::unique_ptr<std::string> is iterable? " << corecpp::is_iterable<std::unique_ptr<std::string>>::value << std::endl;
	std::cout << "int* is iterable? " << corecpp::is_iterable<int*>::value << std::endl;
	std::cout << " === " << std::endl;
	std::cout << "std::vector<int> is dereferencable? " << corecpp::is_dereferencable<std::vector<int>>::value << std::endl;
	std::cout << "std::string is dereferencable? " << corecpp::is_dereferencable<std::string>::value << std::endl;
	std::cout << "std::optional<int> is dereferencable? " << corecpp::is_dereferencable<std::optional<int>>::value << std::endl;
	std::cout << "std::unique_ptr<std::string> is dereferencable? " << corecpp::is_dereferencable<std::unique_ptr<std::string>>::value << std::endl;
	std::cout << " === " << std::endl;
	std::cout << "int* is nothrow_equality_comparable? " << corecpp::is_nothrow_equality_comparable<int*>::value << std::endl;
	std::cout << "std::vector<int> is nothrow_equality_comparable? " << corecpp::is_nothrow_equality_comparable<std::vector<int>>::value << std::endl;
	std::cout << "std::string is nothrow_equality_comparable? " << corecpp::is_nothrow_equality_comparable<std::string>::value << std::endl;
	std::cout << "std::optional<int> is nothrow_equality_comparable? " << corecpp::is_nothrow_equality_comparable<std::optional<int>>::value << std::endl;
	std::cout << "std::unique_ptr<std::string> is nothrow_equality_comparable? " << corecpp::is_nothrow_equality_comparable<std::unique_ptr<std::string>>::value << std::endl;
	std::cout << "int* is nothrow_equality_comparable? " << corecpp::is_nothrow_equality_comparable<int*>::value << std::endl;
	std::cout << " === " << std::endl;

	/*
	TODO: is_associative
	*/

	std::cout << "std::vector<int> represent a point in time? " << corecpp::is_time_point<std::vector<int>>::value << std::endl;
	std::cout << "std::string represent a point in time? " << corecpp::is_time_point<std::string>::value << std::endl;
	std::cout << "std::optional<int> represent a point in time? " << corecpp::is_time_point<std::optional<int>>::value << std::endl;
	std::cout << "std::unique_ptr<int> represent a point in time? " << corecpp::is_time_point<std::unique_ptr<int>>::value << std::endl;
	std::cout << "int* represent a point in time? " << corecpp::is_time_point<int*>::value << std::endl;
	std::cout << "std::chrono::steady_clock::time_point represent a point in time? " << corecpp::is_time_point<std::chrono::steady_clock::time_point>::value << std::endl;


	return EXIT_SUCCESS;
};

