#pragma once

template<class T>
T NullableNullValue();
template<>
int NullableNullValue();
template<>
float NullableNullValue();

template<class T>
bool NullableIsNull(T x);
template<>
bool NullableIsNull(int x);
template<>
bool NullableIsNull(float x);
