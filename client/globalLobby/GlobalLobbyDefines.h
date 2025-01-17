/*
 * GlobalLobbyClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct GlobalLobbyAccount
{
	std::string accountID;
	std::string displayName;
	std::string status;
};

struct GlobalLobbyRoom
{
	std::string gameRoomID;
	std::string hostAccountID;
	std::string hostAccountDisplayName;
	std::string description;
	int playersCount;
	int playersLimit;
};
