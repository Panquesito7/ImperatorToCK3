#include "Characters.h"
#include "Character.h"
#include "../Genes/GenesDB.h"
#include "../Families/Families.h"
#include "Log.h"
#include "ParserHelpers.h"
#include "CommonRegexes.h"
#include <set>
#include <utility>
#include <ranges>



Imperator::Characters::Characters(std::istream& theStream, const std::shared_ptr<GenesDB>& genesDB): genes(genesDB) {
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}


void Imperator::Characters::registerKeys() {
	registerRegex(commonItems::integerRegex, [this](const std::string& charID, std::istream& theStream) {
		std::shared_ptr<Character> newCharacter = characterFactory.getCharacter(theStream, charID, genes);
		characters.emplace(newCharacter->getID(), newCharacter);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}


void Imperator::Characters::linkFamilies(const Families& theFamilies) {
	auto counter = 0;
	std::set<unsigned long long> idsWithoutDefinition;
	const auto& families = theFamilies.getFamilies();

	for (const auto& character : characters | std::views::values) {
		auto familyID = character->getFamily().first;
		if (const auto& familyItr = families.find(familyID); familyItr != families.end()) {
			character->setFamily(familyItr->second);
			familyItr->second->linkMember(character);
			++counter;
		}
		else {
			idsWithoutDefinition.emplace(familyID);
		}
	}

	std::string warningString = "Families without definition:";
	if (!idsWithoutDefinition.empty()) {
		for (auto id : idsWithoutDefinition) {
			warningString += " ";
			warningString += std::to_string(id);
			warningString += ",";
		}
		warningString = warningString.substr(0, warningString.size() - 1); //remove last comma
		Log(LogLevel::Warning) << warningString;
	}
	
	Log(LogLevel::Info) << "<> " << counter << " families linked to characters.";
}


void Imperator::Characters::linkSpouses() {
	auto counterSpouse = 0;
	for (const auto& character : characters | std::views::values) {
		if (!character->getSpouses().empty()) {
			std::map<unsigned long long, std::shared_ptr<Character>> newSpouses;
			for (const auto& spouseID : character->getSpouses() | std::views::keys) {
				const auto& characterItr = characters.find(spouseID);
				if (characterItr != characters.end()) {
					newSpouses.emplace(characterItr->first, characterItr->second);
					++counterSpouse;
				}
				else {
					Log(LogLevel::Warning) << "Spouse ID: " << spouseID << " has no definition!";
				}
			}
			character->setSpouses(newSpouses);
		}
	}
	Log(LogLevel::Info) << "<> " << counterSpouse << " spouses linked.";
}


void Imperator::Characters::linkMothersAndFathers() {
	auto counterMother = 0;
	auto counterFather = 0;
	for (const auto& character: characters) {
		if (character.second->getMother().first) {
			const auto& characterItr = characters.find(character.second->getMother().first);
			if (characterItr != characters.end()) {
				character.second->setMother(std::pair(characterItr->first, characterItr->second));
				characterItr->second->registerChild(character);
				++counterMother;
			}
			else {
				Log(LogLevel::Warning) << "Mother ID: " << character.second->getMother().first << " has no definition!";
			}
		}

		if (character.second->getFather().first) {
			const auto& characterItr = characters.find(character.second->getFather().first);
			if (characterItr != characters.end()) {
				character.second->setFather(std::pair(characterItr->first, characterItr->second));
				characterItr->second->registerChild(character);
				++counterFather;
			}
			else {
				Log(LogLevel::Warning) << "Father ID: " << character.second->getFather().first << " has no definition!";
			}
		}
	}
	Log(LogLevel::Info) << "<> " << counterMother << " mothers and " << counterFather << " fathers linked.";
}




Imperator::CharactersBloc::CharactersBloc(std::istream& theStream, const GenesDB& genesDB): genes(std::make_shared<GenesDB>(genesDB)) {
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}


void Imperator::CharactersBloc::registerKeys() {
	registerKeyword("character_database", [this](std::istream& theStream) {
		characters = Characters(theStream, genes);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}