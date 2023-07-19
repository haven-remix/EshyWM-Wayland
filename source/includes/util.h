
#pragma once

#include <iostream>

#define check(condition, message)        if (!(condition)) {std::cout << message << std::endl; abort();}