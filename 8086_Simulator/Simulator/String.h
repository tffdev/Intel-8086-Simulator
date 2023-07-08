#pragma once

class String {
public:
	String();
	String(const char* str);
	String(const String& other);
	~String();
	const char* c_str() const;
	String Clone();
	static String Format(String format, ...);
	void Set(String str);
private:
	int* refCount;
	char* data;
	int length;
};