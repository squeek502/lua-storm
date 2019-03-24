#include "lauxlib.h"
#include "StormLib.h"

#ifdef _WIN32
	#include <windows.h>
	#define EXPORT __declspec(dllexport)
#else
	#include <unistd.h>
	#define EXPORT
#endif

#if LUA_VERSION_NUM < 502
#ifndef luaL_newlib
#	define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif
#	define luaL_setfuncs(L,l,n) (luaL_register(L,NULL,l))
#endif

static int push_error(lua_State *L, const char* msg)
{
	lua_pushnil(L);
	lua_pushstring(L, msg);
	return 2;
}

static int push_last_error(lua_State *L)
{
#ifdef _WIN32
	char err[256];
	int strLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		GetSystemDefaultLangID(), // Default language
		err, 256, NULL
	);
	err[strLen - 2] = '\0'; // strip the \r\n
	return push_error(L, err);
#else
	return push_error(L, strerror(GetLastError()));
#endif
}

#define MPQ_T "storm.mpq"
typedef struct {
	HANDLE handle;
	TCHAR name[MAX_PATH];
} mpq_t;

static mpq_t* push_mpq(lua_State *L)
{
	mpq_t *mpq = (mpq_t*)lua_newuserdata(L, sizeof(mpq_t));
	luaL_getmetatable(L, MPQ_T);
	lua_setmetatable(L, -2);
	return mpq;
}

static mpq_t* check_mpq(lua_State *L, int index)
{
	mpq_t* mpq = (mpq_t*)luaL_checkudata(L, index, MPQ_T);
	return mpq;
}

static void mpq_init(mpq_t* mpq, HANDLE handle, const char* name)
{
	mpq->handle = handle;
	strcpy(mpq->name, name);
}

static int mpq_read(lua_State *L)
{
	mpq_t* mpq = check_mpq(L, 1);
	const char* filename = luaL_checkstring(L, 2);

	HANDLE file;
	if (!SFileOpenFileEx(mpq->handle, filename, 0, &file))
		return push_last_error(L);

	DWORD fileSize = SFileGetFileSize(file, NULL);
	BYTE* buf = malloc(fileSize);
	if (!buf)
	{
		SFileCloseFile(file);
		return push_error(L, "failed to allocate buffer for file contents");
	}

	DWORD bytesRead;
	if (!SFileReadFile(file, buf, fileSize, &bytesRead, NULL))
	{
		SFileCloseFile(file);
		free(buf);
		return push_last_error(L);
	}

	lua_pushlstring(L, buf, bytesRead);

	SFileCloseFile(file);
	free(buf);

	return 1;
}

static int mpq_files(lua_State *L)
{
	mpq_t* mpq = check_mpq(L, 1);

	SFILE_FIND_DATA findData;
	HANDLE find = SFileFindFirstFile(mpq->handle, "*", &findData, NULL);
	if (!find)
		return push_last_error(L);

	lua_newtable(L);
	int i = 1;
	do
	{
		lua_newtable(L);
		lua_pushstring(L, findData.cFileName);
		lua_setfield(L, -2, "name");
		lua_pushnumber(L, findData.dwFileSize);
		lua_setfield(L, -2, "size");
		lua_pushstring(L, findData.szPlainName);
		lua_setfield(L, -2, "basename");
		lua_rawseti(L, -2, i++);
	}
	while (SFileFindNextFile(find, &findData));

	SFileFindClose(find);

	return 1;
}

static int mpq_extract(lua_State *L)
{
	mpq_t* mpq = check_mpq(L, 1);
	const char* filenameToExtract = luaL_checkstring(L, 2);
	const char* filenameOut = luaL_checkstring(L, 3);

	if (!SFileExtractFile(mpq->handle, filenameToExtract, filenameOut, 0))
		return push_last_error(L);

	lua_pushboolean(L, 1);
	return 1;
}

static int mpq_gc(lua_State *L)
{
	mpq_t* mpq = check_mpq(L, 1);
	SFileCloseArchive(mpq->handle);
	return 0;
}

static const luaL_Reg mpq_funcs[] = {
	{ "read", mpq_read },
	{ "files", mpq_files },
	{ "extract", mpq_extract },
	{ "__gc", mpq_gc },
	{ NULL, NULL }
};

static void register_mpq(lua_State *L)
{
	luaL_newmetatable(L, MPQ_T);
	luaL_setfuncs(L, mpq_funcs, 0);
	lua_pushvalue(L, -1); // re-push the metatable
	lua_setfield(L, -2, "__index"); // set it as it's own __index so that methods can be found
	lua_pop(L, 1);
}

static int luastorm_open(lua_State *L)
{
	HANDLE hMpq;
	size_t len;
	const char* filename = luaL_checklstring(L, 1, &len);

	if (len >= MAX_PATH)
		return push_error(L, "path too long");

	if (!SFileOpenArchive(filename, 0, MPQ_OPEN_READ_ONLY, &hMpq))
		return push_last_error(L);

	mpq_t* mpq = push_mpq(L);
	mpq_init(mpq, hMpq, filename);
	return 1;
}

static const luaL_Reg storm_lualib[] = {
	{ "open", luastorm_open },
	{ NULL, NULL }
};

EXPORT int luaopen_storm(lua_State *L)
{
	register_mpq(L);
	luaL_newlib(L, storm_lualib);
	return 1;
}