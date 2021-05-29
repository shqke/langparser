#include "context.h"

#include <wctype.h>
#ifdef POSIX
#include <iconv.h>
#endif // POSIX

const char* TokenTypeToStr(Token_t tokenType)
{
	switch (tokenType) {
	case Token_String:
		return "TOKEN_STRING";
	case Token_SectionBegin:
		return "TOKEN_NEW_SECTION";
	case Token_SectionEnd:
		return "TOKEN_SECTION_END";
	case Token_Condition:
		return "TOKEN_CONDITION";
	}

	return "UNKNOWN";
}

int UCS2ToUTF8(const ucs2_t* pUCS2, char* pUTF8, int cubDestSizeInBytes)
{
	pUTF8[0] = 0;
#ifdef _WIN32
	int cchResult = WideCharToMultiByte(CP_UTF8, 0, pUCS2, -1, pUTF8, cubDestSizeInBytes, NULL, NULL);
#elif defined(POSIX)
	iconv_t conv_t = iconv_open("UTF-8", "UCS-2LE");
	size_t cchResult = -1;
	size_t nLenUnicde = cubDestSizeInBytes;
	size_t nMaxUTF8 = cubDestSizeInBytes;
	char* pIn = (char*)pUCS2;
	char* pOut = (char*)pUTF8;
	if (conv_t != (iconv_t)-1)
	{
		cchResult = 0;
		cchResult = iconv(conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8);
		iconv_close(conv_t);
		if ((int)cchResult < 0)
			cchResult = 0;
		else
			cchResult = nMaxUTF8;
	}
#endif
	pUTF8[cubDestSizeInBytes - 1] = 0;
	return cchResult;
}

bool IsUnicodeWhitespaceCharacter(ucs2_t wch)
{
	if (iswspace(wch) != 0) {
		return true;
	}

	// Left 4 Dead 1 occasionally uses U+00A0 in lang files as whitespace
	// added others just in case (tokens should be enquoted anyway)
	// https://en.wikipedia.org/wiki/Whitespace_character
	static const ucs2_t s_wwsChars[] =
	{
		u'\u0085', // next line
		u'\u00A0', // no-break space
		u'\u1680', // ogham space mark
		u'\u2000', // en quad
		u'\u2001', // em quad
		u'\u2002', // en space
		u'\u2003', // em space
		u'\u2004', // three-per-em space
		u'\u2005', // four-per-em space
		u'\u2006', // six-per-em space
		u'\u2007', // figure space
		u'\u2008', // punctuation space
		u'\u2009', // thin space
		u'\u200A', // hair space
		u'\u2028', // line separator
		u'\u2029', // paragraph separator
		u'\u202F', // narrow no-break space
		u'\u205F', // medium mathematical space
		u'\u3000', // ideographic space
	};

	for (auto wwsch : s_wwsChars) {
		if (wwsch >= wch) {
			return wwsch == wch;
		}
	}

	return false;
}

ParserContext::ParserContext()
{
	Reset();
}

void ParserContext::Reset()
{
	m_pCur = NULL;
	m_pInputBuffer = NULL;

	m_pErrorString = NULL;
	m_nMaxlength = 0;
}

void ParserContext::SetError(ParseError_t nError, const char* fmt, ...)
{
	m_nErrorCode = nError;

	if (fmt != NULL) {
		va_list args;
		va_start(args, fmt);
		V_vsnprintf(m_pErrorString, m_nMaxlength, fmt, args);
		va_end(args);
	}
}

void ParserContext::AdvanceOverWhitespace()
{
	while (*m_pCur != u'\0' && IsUnicodeWhitespaceCharacter(*m_pCur)) {
		m_pCur++;
	}
}

void ParserContext::AdvanceToLineEnd()
{
	while (*m_pCur != u'\0' && *(m_pCur++) != u'\n') {
		//
	}
}

ParseError_t ParserContext::Process(const ucs2_t* pInputBuffer, ILanguageFileParserListener* pListener, char* error, size_t maxlength)
{
	Reset();

	m_pCur = m_pInputBuffer = pInputBuffer;

	m_nErrorCode = ParseError_None;
	m_pErrorString = error;
	m_nMaxlength = maxlength;

	char token[MAX_KEY_LENGTH] = {};
	char key[MAX_KEY_LENGTH] = {};
	char value[MAX_PHRASE_LENGTH] = {};

	size_t depth = 0;

	enum State_t
	{
		// Just started, had a condition, ended or started section
		State_Default,
		State_GotKey,
		State_GotValue,
	};

	State_t newState = State_Default;
	ParseAction_t nAction = pListener->State_Started();

	while (1)
	{
		if (m_nErrorCode != ParseError_None) {
			break;
		}

		if (nAction != Parse_Continue) {
			break;
		}

		const State_t curState = newState;

		ucs2_t ucs2token[sizeof(value)] = {};
		const Token_t tokenType = ReadToken(ucs2token, curState == State_GotKey ? sizeof(value) : sizeof(token));

		// By default it's a token
		char* pszToken = token;

		// Flush old key/value if new key is expected
		if (curState == State_GotValue && (tokenType == Token_String || tokenType == Token_SectionBegin || tokenType == Token_SectionEnd)) {
			nAction = pListener->State_KeyValue(key, value);
			if (nAction != Parse_Continue) {
				continue;
			}
		}

		if (tokenType == Token_String) {
			if (curState == State_Default || curState == State_GotValue) {
				pszToken = key;
				newState = State_GotKey;
			}
			else if (curState == State_GotKey) {
				pszToken = value;
				newState = State_GotValue;
			}
		}
		else if (tokenType == Token_Condition) {
			// Condition is not yet converted into UTF8, so let passthrough
		}
		else if (tokenType == Token_SectionBegin) {
			if (curState == State_GotKey || curState == State_GotValue) {
				newState = State_Default;
				depth++;

				nAction = pListener->State_EnteredSection(key);
				if (nAction != Parse_Continue) {
					continue;
				}
			}
			else {
				SetError(ParseError_SectionBegin, "Unexpected new section");
			}
		}
		else if (tokenType == Token_SectionEnd) {
			if (curState == State_GotValue || curState == State_Default) {
				if (depth == 0) {
					SetError(ParseError_SectionEnd, "Unexpected section end");
				}
				else {
					newState = State_Default;
					depth--;

					nAction = pListener->State_LeftSection();
					if (nAction == Parse_Continue) {
						if (depth == 0) {
							break;
						}
					}
				}
			}
		}
		//

		// rww: may fail?
		UCS2ToUTF8(ucs2token, pszToken, pszToken == value ? sizeof(value) : sizeof(token));

		if (m_nErrorCode != ParseError_None) {
			// If we have an error - stop now
			continue;
		}

		if (tokenType == Token_Condition) {
			if (curState == State_GotValue) {
				newState = State_Default;

				if (EvaluateCondition(token)) {
					nAction = pListener->State_KeyValue(key, value);
				}
			}
			else {
				SetError(ParseError_InvalidToken, "Unexpected condition '%s'", token);
				continue;
			}
		}

		DevMsg(2, "new state: %u key: '%s' value: '%s' token: %u ('%s')\n", newState, key, value, tokenType, pszToken);
	}

	pListener->State_Ended(nAction != Parse_Continue, nAction == Parse_HaltFail || m_nErrorCode != ParseError_None);
	return m_nErrorCode;
}

Token_t ParserContext::ReadToken(ucs2_t* pOut, size_t maxlength)
{
	while (1) {
		// Skip over any whitespace
		AdvanceOverWhitespace();

		if (*m_pCur == u'/' && *(m_pCur + 1) == u'/') {
			// Skip comments
			AdvanceToLineEnd();
			continue;
		}

		break;
	}

	enum Mode_t
	{
		Mode_Quoted,
		Mode_Unquoted,
		Mode_Condition,
	};

	Mode_t nMode = Mode_Unquoted;
	if (*m_pCur == u'"') {
		m_pCur++;
		nMode = Mode_Quoted;
	}
	else if (*m_pCur == u'[') {
		m_pCur++;
		nMode = Mode_Condition;
	}
	else {
		auto ucs2c = *m_pCur;
		if (ucs2c == u'{' || ucs2c == u'}') {
			m_pCur++;

			*(pOut++) = ucs2c;
			*(pOut++) = u'\0';

			return ucs2c == u'{' ? Token_SectionBegin : Token_SectionEnd;
		}
	}
	
	size_t count = 0;

	while (1) {
		auto ucs2c = *(m_pCur++);
		if (ucs2c == u'\0') {
			SetError(ParseError_StreamEnd, "Encountered NULL terminator");
			break;
		}

		if (nMode == Mode_Quoted) {
			if (ucs2c == u'"') {
				break;
			}

			// Check for special characters
			if (ucs2c == u'\\') {
				if (*m_pCur == u'\\') {
					m_pCur++;
					ucs2c = u'\\';
				}
				else if (*m_pCur == u'"') {
					m_pCur++;
					ucs2c = u'"';
				}
				else if (*m_pCur == u'n') {
					m_pCur++;
					ucs2c = u'\n';
				}
				else if (*m_pCur == u'r') {
					m_pCur++;
					ucs2c = u'\r';
				}
				else if (*m_pCur == u't') {
					m_pCur++;
					ucs2c = u'\t';
				}
			}
		}
		else if (nMode == Mode_Condition) {
			if (ucs2c == u']') {
				break;
			}
		}
		else if (nMode == Mode_Unquoted) {
			if (IsUnicodeWhitespaceCharacter(ucs2c)) {
				break;
			}

			if (ucs2c == u'"') {
				break;
			}

			if (ucs2c == u'[') {
				break;
			}
		}

		if (count + 1 == maxlength) {
			SetError(ParseError_Overflow, "Token buffer overflow");
			break;
		}

		*(pOut++) = ucs2c;
		count++;
	}

	*pOut = u'\0';
	return nMode == Mode_Condition ? Token_Condition : Token_String;
}

bool ParserContext::EvaluateCondition(const char* pszCond)
{
	if (*pszCond == '!') {
		return !EvaluateCondition(pszCond + 1);
	}

	if (*pszCond == '$') {
		if (V_stricmp(pszCond + 1, "WIN32") == 0) {
			return true;
		}
		else if (V_stricmp(pszCond + 1, "X360") == 0) {
			return false;
		}
		else if (V_stricmp(pszCond + 1, "PS3") == 0) {
			return false;
		}
#if SOURCE_ENGINE != SE_EPISODEONE
		else if (V_stricmp(pszCond + 1, "LOWVIOLENCE") == 0) {
			return engine->IsLowViolence();
		}
#endif
	}

	DevWarning("Unknown condition \"%s\"\n", pszCond);
	return false;
}
