#ifndef FILEBASE_H
#define FILEBASE_H
#include <sys/stat.h>
#include <string>
#include <memory>

struct FileBase
{
	FileBase(std::string path);

	bool moreRecent( FileBase const&) const;
	bool moreRecent( time_t time) const;

	bool ChangePath(const char *);
	bool ChangePath(std::string &&);

	std::string path;
	time_t      modified{};
	bool        isLoaded{};

	bool doesExist();

private:
	void update();

	bool m_exists{};
};

struct FileDeleter
{
	FileDeleter() = default;
	void operator()(FILE * fp) const { if(fp != nullptr) fclose(fp); }
};

typedef std::unique_ptr<FILE, FileDeleter> FileHandle;

#endif // FILEBASE_H
