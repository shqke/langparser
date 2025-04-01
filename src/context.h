#ifndef _INCLUDE_CONTEXT_H_
#define _INCLUDE_CONTEXT_H_

#include "smsdk_ext.h"
#include "../ILanguageFileParser.h"

enum Token_t
{
	Token_String,
	Token_SectionBegin,
	Token_SectionEnd,
	Token_Condition,
};

struct ParserContext
{
	// context
	const ucs2_t* m_pCur;
	const ucs2_t* m_pInputBuffer;

	ParseError_t m_nErrorCode;
	char* m_pErrorString;
	size_t m_nMaxlength;

	ParserContext();

	void Reset();
	void SetError(ParseError_t nError, const char* fmt, ...);

	void AdvanceOverWhitespace();
	void AdvanceToLineEnd();

	ParseError_t Process(const ucs2_t* pInputBuffer, ILanguageFileParserListener* pListener, char* error, size_t maxlength);

	Token_t ReadToken(ucs2_t* pszOut, size_t maxlength);
	bool EvaluateCondition(const char* pszCond);
};

#endif // _INCLUDE_CONTEXT_H_
