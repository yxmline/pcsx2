/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <string>

enum GamefixId;
enum SpeedhackId;

class GameDatabaseSchema
{
public:
	enum class Compatibility
	{
		Unknown = 0,
		Nothing,
		Intro,
		Menu,
		InGame,
		Playable,
		Perfect
	};

	enum class RoundMode
	{
		Undefined = -1,
		Nearest = 0,
		NegativeInfinity,
		PositiveInfinity,
		ChopZero
	};

	enum class ClampMode
	{
		Undefined = -1,
		Disabled = 0,
		Normal,
		Extra,
		Full
	};

	using Patch = std::vector<std::string>;

	struct GameEntry
	{
		std::string name;
		std::string region;
		Compatibility compat = Compatibility::Unknown;
		RoundMode eeRoundMode = RoundMode::Undefined;
		RoundMode vuRoundMode = RoundMode::Undefined;
		ClampMode eeClampMode = ClampMode::Undefined;
		ClampMode vuClampMode = ClampMode::Undefined;
		std::vector<GamefixId> gameFixes;
		std::vector<std::pair<SpeedhackId, int>> speedHacks;
		std::vector<std::string> memcardFilters;
		std::unordered_map<std::string, Patch> patches;

		// Returns the list of memory card serials as a `/` delimited string
		std::string memcardFiltersAsString() const;
		const Patch* findPatch(const std::string_view& crc) const;
		const char* compatAsString() const;
	};
};

namespace GameDatabase
{
	void ensureLoaded();
	const GameDatabaseSchema::GameEntry* findGame(const std::string_view& serial);
}; // namespace GameDatabase
