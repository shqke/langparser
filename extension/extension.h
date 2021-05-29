#ifndef _INCLUDE_LANGPARSER_EXT_H_
#define _INCLUDE_LANGPARSER_EXT_H_

#include "smsdk_ext.h"
#include "../ILanguageFileParser.h"

class CExtension :
	public SDKExtension,
	public ILanguageFileParser
{
public: // ILanguageFileParser
	ParseError_t ParseFile(const char* pszRelativePath, ILanguageFileParserListener* pListener, char* error, size_t maxlength) override;
	ParseError_t ParseBuffer(const ucs2_t* pDataIn, ILanguageFileParserListener* pListener, char* error, size_t maxlength) override;

public: // SDKExtension
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	bool SDK_OnLoad(char* error, size_t maxlen, bool late) override;

	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	bool SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
};

#endif // _INCLUDE_LANGPARSER_EXT_H_
