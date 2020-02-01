#include "filebase.h"

FileBase::FileBase(std::string path)
{
	ChangePath(std::move(path));
}

bool FileBase::moreRecent( FileBase const& it) const
{
	if(it.m_exists == false)
		return m_exists;

	return (modified > it.modified);
}

bool FileBase::moreRecent( time_t time) const
{
	return (modified >= time);
}


void FileBase::update()
{
	struct stat buf;
	if(0 == stat(path.c_str(), &buf))
	{
		m_exists = true;
		modified = buf.st_mtime;
	}
	else
	{
		m_exists = false;
		modified = 0;
	}
}

bool FileBase::doesExist()
{
	update();
	return m_exists;
}

bool FileBase::ChangePath(const char * p)
{
	path = p;
	update();

	return m_exists;
}

bool FileBase::ChangePath(std::string && p)
{
	path = std::move(p);
	update();

	return m_exists;
}
