// $Id$

#ifndef __INFOCOMMAND_HH__
#define __INFOCOMMAND_HH__

#include <map>
#include "Command.hh"
#include "InfoTopic.hh"

namespace openmsx {

class InfoCommand : public Command
{
public:
	static InfoCommand& instance();
	void registerTopic(const std::string& name, const InfoTopic* topic);
	void unregisterTopic(const std::string& name, const InfoTopic* topic);
	
	// Command
	virtual void execute(const std::vector<CommandArgument>& tokens,
	                     CommandArgument& result);
	virtual std::string help(const std::vector<std::string>& tokens) const;
	virtual void tabCompletion(std::vector<std::string>& tokens) const;

private:
	InfoCommand();
	virtual ~InfoCommand();

	std::map<std::string, const InfoTopic*> infoTopics;

	class VersionInfo : public InfoTopic {
	public:
		virtual void execute(const std::vector<CommandArgument>& tokens,
		                     CommandArgument& result) const;
		virtual std::string help(const std::vector<std::string>& tokens) const;
	} versionInfo;
};

} // namespace openmsx

#endif
