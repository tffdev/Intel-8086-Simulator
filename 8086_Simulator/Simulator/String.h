#pragma once

class String {
public:
	String();
	String(const char* str);
	String(const String& other);
	~String();
	static String Format(String format, ...);

	const char* c_str() const;
	void operator=(const String& other);
	
	String Clone();
	bool Equals(const char* other);
	void Set(String str);
	
	char* data;

private:
	int* refCount;
	int length;
};