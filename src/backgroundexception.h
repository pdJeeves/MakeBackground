#ifndef BACKGROUNDEXCEPTION_H
#define BACKGROUNDEXCEPTION_H
#include <exception>
#include <string>


class ApplicationException : public std::exception
{
public:
	ApplicationException(const char * text) :
		m_content(text) { }

	ApplicationException(std::string const& text) :
		m_content(text) { }

	ApplicationException(std::string && text) :
		m_content(std::move(text)) { }

	~ApplicationException() = default;

	const char* what() const noexcept override {
		return m_content.c_str();
	}

protected:
	std::string m_content{};
};

#define Exception_Blueprint(name, parent) \
class name : public parent \
{\
public: \
	name() = default; \
	name(const char * text) : parent(text) { } \
	name(std::string const& text) :	parent(text) { } \
	name(std::string && text) :	parent(std::move(text)) { } \
	~name() = default; \
}

Exception_Blueprint(LibPngException, ApplicationException);
Exception_Blueprint(FileException, ApplicationException);
Exception_Blueprint(BackgroundException, ApplicationException);
Exception_Blueprint(DDSException, ApplicationException);


#endif // BACKGROUNDEXCEPTION_H
