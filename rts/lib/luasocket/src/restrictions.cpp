/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "restrictions.h"

#include <map>
#include <string>

#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"

#define LOG_SECTION_LUASOCKET "LuaSocket"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_LUASOCKET)
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_LUASOCKET

CONFIG(std::string, TCPAllowConnect).defaultValue("").readOnly(true);
CONFIG(std::string, TCPAllowListen).defaultValue("").readOnly(true);
CONFIG(std::string, UDPAllowConnect).defaultValue("").readOnly(true);
CONFIG(std::string, UDPAllowListen).defaultValue("").readOnly(true);

CLuaSocketRestrictions* luaSocketRestrictions=0;

CLuaSocketRestrictions::CLuaSocketRestrictions()
{
	addRules(TCP_CONNECT, configHandler->GetString("TCPAllowConnect"));
	addRules(TCP_LISTEN,  configHandler->GetString("TCPAllowListen"));
	addRules(UDP_CONNECT, configHandler->GetString("UDPAllowConnect"));
	addRules(UDP_LISTEN,  configHandler->GetString("UDPAllowListen"));

}

void CLuaSocketRestrictions::addRule(RestrictType type, const std::string& hostname, int port, bool allowed)
{
	if ((port<=0) || (port>65535)){
		LOG_L(L_ERROR, "Invalid port specified: %d", port);
		return;
	}
	if (getRule(type, hostname.c_str(), port)!=NULL) {
		LOG_L(L_ERROR, "Rule already exists: %s %d", hostname.c_str(), port);
		return;
	}
	LOG_L(L_WARNING, "Adding rule %d %s:%d",type, hostname.c_str(), port);
	if (!allowed) { //add deny rules to the front of the list
		restrictions[type].push_front(TSocketRule(hostname, port, allowed));
	} else {
		restrictions[type].push_back(TSocketRule(hostname, port, allowed));
	}
}

void CLuaSocketRestrictions::addRule(RestrictType type, const std::string& rule)
{
	size_t delimpos = rule.find(":");
	if (delimpos==std::string::npos) {
		LOG_L(L_ERROR, "Invalid %d rule: %s, rule has to be hostname:port", type, rule.c_str());
		return;
	}
	const std::string strport = rule.substr(delimpos+1, delimpos-rule.length());
	const int port = atoi(strport.c_str());
	addRule(type, rule.substr(0, delimpos), port, true);
}

void CLuaSocketRestrictions::addRules(RestrictType type, const std::string& configstr)
{
	int i=0;
	char ch;
	std::string rule;

	while((ch=configstr[i++])) {
		if ((isspace(ch)) && (!rule.empty())) {
			addRule(type, rule);
			rule = "";
		} else {
			rule += ch;
		}
	}
	if (!rule.empty()) {
		addRule(type, rule);
	}
}

/*
bool isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}
*/

bool CLuaSocketRestrictions::isAllowed(RestrictType type, const char* hostname, int port) {
	const TSocketRule* rule = getRule(type, hostname, port);
	if (rule==NULL) {
		return false;
	}
	return rule->allowed;
}

const TSocketRule* CLuaSocketRestrictions::getRule(RestrictType type, const char* hostname, int port) {
{
	int start, end;
	if (type != ALL_RULES) {
		start = type;
		end = start+1;
	} else {
		start = 0;
		end = ALL_RULES;
	}
	for (int i=start; i<end; i++) {
		TStrIntMap::iterator it;
		for(it = restrictions[i].begin(); it != restrictions[i].end(); ++it) {
			TSocketRule &rule = *it;
			if (hostname == rule.hostname) {
				if (rule.port==-1) { // port ignored
					return &rule;
				} else if (rule.port==port) {
					return &rule;
				}
			}
		}
	}
	return NULL;
	}
}

void CLuaSocketRestrictions::addIP(const char* hostname, const char* ip)
{
	for(int i=0; i<ALL_RULES; i++) {
		TStrIntMap::iterator it;
		for(it = restrictions[i].begin(); it != restrictions[i].end(); ++it) {
			const TSocketRule &rule = *it;
			if ((rule.hostname == hostname)  && (getRule((RestrictType)i, ip, rule.port) == NULL)) { //add rule with ip, when not exisitng
				addRule((RestrictType)i, ip, rule.port, rule.allowed);
			}
		}
	}
}

CLuaSocketRestrictions::~CLuaSocketRestrictions()
{
	//FIXME: dump rules only in debug build if luasockets has become more stable
	for(int i=0; i<ALL_RULES; i++) {
		TStrIntMap::iterator it;
		for(it = restrictions[i].begin(); it != restrictions[i].end(); ++it) {
			TSocketRule &rule = *it;
			LOG_L(L_WARNING, "%d %s %d %d", i, rule.hostname.c_str(), rule.port, rule.allowed);
		}
	}

}
