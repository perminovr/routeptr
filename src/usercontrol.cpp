#include "usercontrol.h"

using namespace std;


static vector<string> split(const string& str, const string& delim)
{
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}


void UserControl::sendAllMoving(bool start)
{
}


void UserControl::sendAllCalibration(bool start)
{
}


void UserControl::sendAllCtl(bool on)
{
}


void UserControl::sendAllError()
{
}


void UserControl::sendAll(std::string msg)
{
	ByteArray ans(msg.begin(), msg.end());
	server->sendAll(ans);
}


void UserControl::onCtlReadyMsgCb(CtlClient *client)
{
	if (!this->cmdHandler)
		return;
		
	ByteArray arr = client->getMsg();
	string msg((const char *)arr.data(), arr.size());
	auto list = split(msg, " ");
	int sz = list.size();
	if (sz) {
		Command cmd;
		cmd.type = Command::Type::No;

		if (cmd.type != Command::Type::No && this->cmdHandler)
			this->cmdHandler(cmd);
	}
}


UserControl::UserControl(CtlServer *server)
{
	this->server = server;
	server->setReadyMsgCb(this, &UserControl::onCtlReadyMsgCb);
}


UserControl::~UserControl()
{}
