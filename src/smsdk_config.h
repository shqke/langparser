#ifndef _INCLUDE_LANGPARSER_CONFIG_H_
#define _INCLUDE_LANGPARSER_CONFIG_H_

#define SMEXT_CONF_NAME			"Language File Parser"
#define SMEXT_CONF_DESCRIPTION	"Exposes methods to process game localization files"
#define SMEXT_CONF_VERSION		"1.0"
#define SMEXT_CONF_AUTHOR		"Evgeniy \"shqke\" Kazakov"
#define SMEXT_CONF_URL			"https://github.com/shqke/langparser/"
#define SMEXT_CONF_LOGTAG		"langparser"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

#define SMEXT_CONF_METAMOD

#endif // _INCLUDE_LANGPARSER_CONFIG_H_
