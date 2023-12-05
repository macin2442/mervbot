#include "windows_compat.h"

#include <algorithm>
#include <thread>
#include <chrono>
#include <filesystem>
#include "datatypes.h"

#include <cstdio>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <memory>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

class Registry
{
	fs::path registryPath_;
	const std::string registryFileName_{"registry.json"};

	pt::ptree ptree_;

	HKEY hKeyMax_{HKEY_USERS};
	std::map<HKEY, pt::path> openPaths_;

public:
	Registry()
	{
		registryPath_ = fs::path(getenv("HOME")) / fs::path(".local/var/mervbot");
		try
		{
			pt::json_parser::read_json<pt::ptree>(boost::filesystem::canonical(registryPath_ / registryFileName_).string(), ptree_);
		}
		catch (boost::filesystem::filesystem_error &e)
		{
			if (e.code() != boost::system::errc::no_such_file_or_directory)
			{
				throw;
			}
			else
			{
				// ignore non-existant registry file
			}
		}

		openPaths_[HKEY_CLASSES_ROOT] = pt::path("HKEY_CLASSES_ROOT", '\\');
		openPaths_[HKEY_CURRENT_CONFIG] = pt::path("HKEY_CURRENT_CONFIG", '\\');
		openPaths_[HKEY_CURRENT_USER] = pt::path("HKEY_CURRENT_USER", '\\');
		openPaths_[HKEY_LOCAL_MACHINE] = pt::path("HKEY_LOCAL_MACHINE", '\\');
		openPaths_[HKEY_USERS] = pt::path("HKEY_USERS", '\\');
	}

	~Registry()
	{
		flush();
	}

	int openKey(HKEY hKey, const char *subKey, HKEY *result)
	{
		auto it = openPaths_.find(hKey);
		if (it == openPaths_.end())
			return ERROR_INVALID_HANDLE;
		if (subKey == NULL || *subKey == '\0')
		{
			if (result != NULL)
				*result = hKey;

			return ERROR_SUCCESS;
		}

		pt::path keyPath(it->second);
		keyPath /= pt::path(subKey, '\\');

		auto child = ptree_.get_child_optional(keyPath);
		if (child == boost::none)
			return ERROR_PATH_NOT_FOUND;

		hKeyMax_++;
		openPaths_[hKeyMax_] = keyPath;
		if (result != NULL)
			*result = hKeyMax_;

		return ERROR_SUCCESS;
	}

	int closeKey(HKEY hKey)
	{
		if (hKey <= HKEY_USERS)
			return ERROR_INVALID_HANDLE;

		auto it = openPaths_.find(hKey);
		if (it == openPaths_.end())
			return ERROR_INVALID_HANDLE;

		openPaths_.erase(it);

		if (openPaths_.size() <= HKEY_USERS + 1)
		{
			// last handle closed; flush
			flush();
		}

		return ERROR_SUCCESS;
	}

	int queryValue(HKEY hKey, const char *lpValueName, DWORD *lpType, BYTE *lpData, DWORD *lpcbData)
	{
		if (lpcbData == NULL && lpData != NULL)
			return ERROR_BAD_ARGUMENTS;

		auto it = openPaths_.find(hKey);
		if (it == openPaths_.end())
			return ERROR_INVALID_HANDLE;

		pt::path keyPath(it->second);
		keyPath /= pt::path("_values", '\\');

		auto child = ptree_.get_child_optional(keyPath);
		if (child == boost::none)
			return ERROR_FILE_NOT_FOUND;

		auto valueTree = child->get_child_optional(pt::path(lpValueName, '\0'));
		if (valueTree == boost::none)
			return ERROR_FILE_NOT_FOUND;

		auto typeString = valueTree->get<std::string>(pt::path("type"));
		if (typeString == "REG_SZ")
		{
			*lpType = REG_SZ;
			std::string value = valueTree->get<std::string>(pt::path("value"));
			if (lpData == NULL && lpcbData != NULL)
				*lpcbData = value.size() + 1;
			else if (lpData != NULL)
			{
				if (*lpcbData < value.size() + 1)
				{
					*lpcbData = value.size() + 1;
					return ERROR_MORE_DATA;
				}

				*lpcbData = value.size() + 1;
				memcpy(lpData, value.c_str(), value.size() + 1);
			}
		}
		else if (typeString == "REG_DWORD")
		{
			*lpType = REG_DWORD;
			DWORD value = valueTree->get<DWORD>(pt::path("value"));
			if (lpData == NULL && lpcbData != NULL)
				*lpcbData = sizeof(DWORD);
			else if (lpData != NULL)
			{
				if (*lpcbData < sizeof(DWORD))
				{
					*lpcbData = sizeof(DWORD);
					return ERROR_MORE_DATA;
				}

				*lpcbData = sizeof(DWORD);
				*reinterpret_cast<DWORD *>(lpData) = value;
			}
		}
		else
		{
			throw std::invalid_argument("not implemented");
		}

		return ERROR_SUCCESS;
	}

	int createKey(HKEY hKey, const char *subKey, HKEY *result)
	{
		auto it = openPaths_.find(hKey);
		if (it == openPaths_.end())
			return ERROR_INVALID_HANDLE;
		if (subKey == NULL || *subKey == '\0')
		{
			if (result != NULL)
				*result = hKey;

			return ERROR_SUCCESS;
		}

		pt::path keyPath(it->second);
		keyPath /= pt::path(subKey, '\\');

		auto child = ptree_.get_child_optional(keyPath);
		if (child == boost::none)
		{
			// create
			ptree_.put_child(keyPath, pt::ptree());
		}

		hKeyMax_++;
		openPaths_[hKeyMax_] = keyPath;
		if (result != NULL)
			*result = hKeyMax_;

		return ERROR_SUCCESS;
	}

	int setValue(HKEY hKey, const char *valueName, DWORD dwType, const BYTE *data, DWORD cbData)
	{
		auto it = openPaths_.find(hKey);
		if (it == openPaths_.end())
			return ERROR_INVALID_HANDLE;

		pt::path keyPath(it->second);
		keyPath /= pt::path("_values", '\\');

		auto child = ptree_.get_child_optional(keyPath);
		if (child == boost::none)
		{
			child = ptree_.put_child(keyPath, pt::ptree());
		}

		pt::ptree valueTree;
		switch (dwType)
		{
		case REG_SZ:
			if (cbData == 0)
				return ERROR_BAD_ARGUMENTS;
			valueTree.put<std::string>(pt::path("type"), "REG_SZ");
			valueTree.put<std::string>(pt::path("value"), std::string(reinterpret_cast<const char *>(data), cbData - 1));
			break;
		case REG_DWORD:
			if (cbData < sizeof(DWORD))
				return ERROR_BAD_ARGUMENTS;
			valueTree.put<std::string>(pt::path("type"), "REG_DWORD");
			valueTree.put<DWORD>(pt::path("value"), getLong(data, 0));
			break;
		case REG_BINARY:
		case REG_DWORD_LITTLE_ENDIAN:
		case REG_DWORD_BIG_ENDIAN:
		case REG_EXPAND_SZ:
		case REG_LINK:
		case REG_MULTI_SZ:
		case REG_NONE:
		case REG_QWORD:
		case REG_QWORD_LITTLE_ENDIAN:
		default:
			throw std::invalid_argument("not implemented");
		}

		child->put_child(pt::path(valueName, '\0'), valueTree);

		return ERROR_SUCCESS;
	}

	void flush()
	{
		fs::create_directory(registryPath_);

		try
		{
			pt::json_parser::write_json<pt::ptree>((registryPath_ / registryFileName_).string(), ptree_);
		}
		catch (boost::filesystem::filesystem_error &e)
		{
			throw;
		}
	}
};

std::shared_ptr<Registry> registry;

int RegOpenKey(HKEY hKey, const char *lpSubKey, HKEY *phkResult)
{
	if (!registry)
		registry = std::make_shared<Registry>();

	return registry->openKey(hKey, lpSubKey, phkResult);
}

int RegCloseKey(HKEY hKey)
{
	if (!registry)
		registry = std::make_shared<Registry>();

	return registry->closeKey(hKey);
}

int RegQueryValueEx(HKEY hKey, const char *lpValueName, DWORD *lpReserved, DWORD *lpType, BYTE *lpData, DWORD *lpcbData)
{
	if (!registry)
		registry = std::make_shared<Registry>();

	return registry->queryValue(hKey, lpValueName, lpType, lpData, lpcbData);
}

int RegCreateKey(HKEY hKey, const char *lpSubKey, HKEY *phkResult)
{
	if (!registry)
		registry = std::make_shared<Registry>();

	return registry->createKey(hKey, lpSubKey, phkResult);
}

int RegSetValueEx(HKEY hKey, const char *lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
{
	if (!registry)
		registry = std::make_shared<Registry>();

	return registry->setValue(hKey, lpValueName, dwType, lpData, cbData);
}

// https://www.flipcode.com/archives/_kbhit_for_Linux.shtml
int kbhit()
{
	static const int STDIN = 0;
	static bool initialized = false;

	if (!initialized)
	{
		// Use termios to turn off line buffering
		termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = true;
	}

	int bytesWaiting;
	ioctl(STDIN, FIONREAD, &bytesWaiting);
	return bytesWaiting;
}

void Sleep(DWORD dwMilliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}

uint32_t GetTickCount()
{
	static auto t0 = std::chrono::steady_clock::now();

	auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0);
	return (uint32_t)(dt.count());
}

uint32_t GetCurrentDirectory(uint32_t nBufferLength, char *lpBuffer)
{
	auto cwd = std::filesystem::current_path().string();

	if (lpBuffer == nullptr || nBufferLength == 0)
	{
		return static_cast<uint32_t>(cwd.size() + 1);
	}

	std::strncpy(lpBuffer, cwd.c_str(), nBufferLength);

	if (nBufferLength < cwd.size() + 1)
	{
		// buffer too small
		return static_cast<uint32_t>(cwd.size() + 1);
	}
	else
	{
		// buffer large enough
		return static_cast<uint32_t>(cwd.size());
	}
}

class ini_filter
{
public:
    typedef char char_type;
    typedef boost::iostreams::input_filter_tag category;

    bool begin_{true};
    int saved_{EOF};

    template <typename Source>
    int get(Source &src)
    {
        int c;
        if (saved_ != EOF)
        {
            c = saved_;
            saved_ = EOF;
        }
        else {
            c = boost::iostreams::get(src);
        }

        if (begin_)
        {
            if (c != EOF && c != boost::iostreams::WOULD_BLOCK && !std::isspace((unsigned char)c)) {
                begin_ = false;

                if (c != EOF && c != boost::iostreams::WOULD_BLOCK && (unsigned char)c == '/')
                {
                    c = boost::iostreams::get(src);
                    if (c != EOF && c != boost::iostreams::WOULD_BLOCK && (unsigned char)c == '/')
                    {
                        // we found a leading "//" and replace it by a "#"
                        c = '#';
                    }
                    else
                    {
                        // only a single "/" remember the current character and first return the '/'
                        saved_ = c;
                        c = '/';
                    }
                }
            }
        }
        else
        {
            if (c != EOF && c != boost::iostreams::WOULD_BLOCK && (unsigned char)c == '\n') {
                begin_ = true;
            }
        }

        return c;
    }
};

DWORD GetPrivateProfileString(const char *lpAppName, const char *lpKeyName, const char *lpDefault, char *lpReturnedString, uint32_t nSize, const char *lpFileName)
{
	// lpAppName = name of section containing the key name
	// if null -> copy all section names in the file

	// lpKeyName = name of key
	// if null -> copy all key names in the section

	// lpDefault
	// if null -> copy ""

	if (lpAppName == nullptr || lpKeyName == nullptr)
		throw std::invalid_argument("not implemented");

	const char *default_value = (lpDefault == nullptr ? "" : lpDefault);

	fs::path file(lpFileName);
	pt::ptree ini;

	try
	{
		boost::iostreams::filtering_istream ini_stream;
		ini_stream.push(ini_filter());
		ini_stream.push(boost::iostreams::file_descriptor(boost::filesystem::canonical(file).string()));

		pt::ini_parser::read_ini<pt::ptree>(ini_stream, ini);
	}
	catch (boost::filesystem::filesystem_error &e)
	{
		if (e.code() != boost::system::errc::no_such_file_or_directory)
		{
			return 0;
		}
		else
		{
			// ignore non-existant registry file
		}
	}

	pt::path iniPath(lpAppName);
	iniPath /= lpKeyName;

	auto value_optional = ini.get_optional<std::string>(iniPath);
	if (value_optional == boost::none)
		value_optional = std::string(default_value);

	std::strncpy(lpReturnedString, value_optional->c_str(), nSize);
	if (nSize < value_optional->size() + 1)
	{
		// buffer too small
		lpReturnedString[nSize - 1] = '\0';
		return nSize - 1;
	}
	else
	{
		// buffer large enough
		return static_cast<DWORD>(value_optional->size());
	}
}

BOOL WritePrivateProfileString(const char *lpAppName, const char *lpKeyName, const char *lpString, const char *lpFileName)
{
	fs::path file(lpFileName);
	pt::ptree ini;

	try
	{
		boost::iostreams::filtering_istream ini_stream;
		ini_stream.push(ini_filter());
		ini_stream.push(boost::iostreams::file_descriptor(boost::filesystem::canonical(file).string()));

		pt::ini_parser::read_ini<pt::ptree>(ini_stream, ini);
	}
	catch (boost::filesystem::filesystem_error &e)
	{
		if (e.code() != boost::system::errc::no_such_file_or_directory)
		{
			return 0;
		}
		else
		{
			// ignore non-existant registry file
		}
	}

	pt::path iniPath(lpAppName);
	iniPath /= lpKeyName;

	ini.put<std::string>(iniPath, lpString);

	pt::ini_parser::write_ini<pt::ptree>((file).string(), ini);
	return 1;
}

void *GetProcAddress(HMODULE module, const char *symbol)
{
	return dlsym(module, symbol);
}

bool FreeLibrary(HMODULE module)
{
	return dlclose(module) != 0;
}
