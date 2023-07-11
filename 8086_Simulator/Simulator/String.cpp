//----------------------------------------------
// String
// Internally refcounted to avoid copies yay
//----------------------------------------------
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "String.h"

String::String() {
	refCount = new int(1);
	length = 0;
	data = new char[1];
	data[0] = '\0';
}

String::String(const char* str) {
	refCount = new int(1);
	length = 0;
	while (str[length] != '\0') {
		length++;
	}
	data = new char[length + 1];
	for (int i = 0; i <= length; i++) {
		data[i] = str[i];
	}
}

void String::Set(String other) {
	(*refCount)--;
	if (*refCount == 0) {
		delete[] data;
		delete refCount;
	}
	data = other.data;
	length = other.length;
	refCount = other.refCount;
	(*refCount)++;
}

String::String(const String& other) {
	data = other.data;
	length = other.length;
	refCount = other.refCount;
	(*refCount)++;
}

String::~String() {
	(*refCount)--;
	if (*refCount == 0) {
		delete[] data;
		delete refCount;
	}
}

String String::Clone() {
	String str(new char[length + 1]);
	for (int i = 0; i <= length; i++) {
		str.data[i] = data[i];
	}
	return str;
}

const char* String::c_str() const {
	return data;
}

bool String::Equals(const char* other) {
	int i = 0;
	while (data[i] != '\0' && other[i] != '\0') {
		if (data[i] != other[i]) {
			return false;
		}
		i++;
	}
	return data[i] == '\0' && other[i] == '\0';
}

String String::Format(String format, ...) {
	va_list args;
	va_start(args, format);

	int length = vsnprintf(nullptr, 0, format.c_str(), args);
	va_end(args);

	if (length <= 0) {
		return String("");
	}

	String str(new char[length + 1]);
	va_start(args, format);
	vsnprintf(str.data, str.length + 1, format.c_str(), args);
	va_end(args);
	return str;
}