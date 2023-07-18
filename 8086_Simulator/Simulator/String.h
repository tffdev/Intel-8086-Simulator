#pragma once

class String {
public:
	String();
	String(const char* str);
	String(const String& other);
	~String();
	const char* c_str() const;
	String Clone();
	bool Equals(const char* other);
	static String Format(String format, ...);
	void Set(String str);
	void operator=(const String& other);
private:
	int* refCount;
	char* data;
	int length;
};