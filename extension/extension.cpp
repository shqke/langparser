#include "extension.h"
#include "context.h"

#include <filesystem.h>
#include <byteswap.h>

CExtension g_Extension;
SMEXT_LINK(&g_Extension);

IFileSystem* g_pFileSystem = NULL;

ParseError_t CExtension::ParseFile(const char* pszRelativePath, ILanguageFileParserListener* pListener, char* error, size_t maxlength)
{
	FileHandle_t file = g_pFileSystem->Open(pszRelativePath, "rb", "GAME");
	if (file == FILESYSTEM_INVALID_HANDLE) {
		V_snprintf(error, maxlength, "Unable to open file \"%s\"", pszRelativePath);
		return ParseError_StreamOpen;
	}

	int fileSize = g_pFileSystem->Size(file);
	int bufferSize = g_pFileSystem->GetOptimalReadSize(file, fileSize + sizeof(wchar_t));

	ucs2_t* pBuffer = (ucs2_t*)g_pFileSystem->AllocOptimalReadBuffer(file, bufferSize);
	if (pBuffer == NULL) {
		g_pFileSystem->Close(file);

		V_snprintf(error, maxlength, "Unable to allocate buffer (path: \"%s\", buffer size: %u)", pszRelativePath, bufferSize);
		return ParseError_StreamRead;
	}

	if (g_pFileSystem->ReadEx(pBuffer, bufferSize, fileSize, file) == 0) {
		g_pFileSystem->FreeOptimalReadBuffer(pBuffer);
		g_pFileSystem->Close(file);

		V_snprintf(error, maxlength, "Unable to read from file \"%s\" (buffer size: %u)", pszRelativePath, bufferSize);
		return ParseError_StreamRead;
	}

	g_pFileSystem->Close(file);

	// null-terminate the stream
	pBuffer[fileSize / sizeof(ucs2_t)] = u'\0';

	if (LittleShort(*pBuffer) != u'\ufeff') {
		g_pFileSystem->FreeOptimalReadBuffer(pBuffer);

		V_snprintf(error, maxlength, "Missing BOM (path: \"%s\")", pszRelativePath);
		return ParseError_StreamRead;
	}

	//Msg("File size: %uB\n", fileSize);
	//Msg("Buffer size: %uB\n", bufferSize);

	CByteswap byteSwap;
	byteSwap.SetTargetBigEndian(false);
	byteSwap.SwapBufferToTargetEndian(pBuffer, pBuffer, fileSize / sizeof(ucs2_t));

	ParseError_t result = CExtension::ParseBuffer(pBuffer + 1, pListener, error, maxlength);
	g_pFileSystem->FreeOptimalReadBuffer(pBuffer);
	return result;
}

ParseError_t CExtension::ParseBuffer(const ucs2_t* pDataIn, ILanguageFileParserListener* pListener, char* error, size_t maxlength)
{
	ParserContext parserCtx;
	return parserCtx.Process(pDataIn, pListener, error, maxlength);
}

bool CExtension::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	sharesys->AddInterface(myself, this);
	return true;
}

bool CExtension::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	return true;
}
