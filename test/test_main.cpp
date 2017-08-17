#include "cpptest\cpptest.h"

int main()
{
	TestResult result;
	getTestRegistry()->run(result);
}