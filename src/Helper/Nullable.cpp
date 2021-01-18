#include "Helper/Nullable.h"

#include <climits>
#include <cmath>

template<>
int NullableNullValue()
{
    return INT_MIN;
}

template<>
float NullableNullValue()
{
    return NAN;
}

template<>
bool NullableIsNull(int x)
{
    return x == INT_MIN;
}

template<>
bool NullableIsNull(float x)
{
    return std::isnan(x);
}
