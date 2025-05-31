/// @ref core
/// @file glm/vec3.hpp

#include "detail/setup.hpp"

#pragma once
#include <ostream>
#include "detail/type_vec3.hpp"

static std::ostream& operator<<(std::ostream& os, const glm::vec3 & v)  
{  
    os << '(' << v.x << ',' << v.y << ',' << v.z << ')';
	
    return os;  
}