#include <BWAPI.h>
#include <BWAPI/Client.h>
#include "StarterBot.h"
#include "ReplayParser.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <filesystem>

void PlayGame();
void ParseReplay();
void updateIniFile(const std::unordered_map<std::string, std::string>& settings);

int main(int argc, char * argv[])
{
    size_t gameCount = 0;

    // Update the BWAPI ini file settings based on the default settings
    // Look in Starbraft/bwapi-data/bwapi.ini for infomation about these settings
    std::unordered_map<std::string, std::string> settings = {
        { "race", "Random" },
        { "auto_menu", "SINGLE_PLAYER" },
        { "map", "maps/(8)Homeworld.scm" },
        { "enemy_count", "7" },
        { "game_type", "FREE_FOR_ALL" }
    };

    updateIniFile(settings);

    // if we are not currently connected to BWAPI, try to reconnect
    while (!BWAPI::BWAPIClient.connect())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 1000 });
    }

    // if we have connected to BWAPI
    while (BWAPI::BWAPIClient.isConnected())
    {
        // the starcraft exe has connected but we need to wait for the game to start
        std::cout << "Waiting for game start\n";
        while (BWAPI::BWAPIClient.isConnected() && !BWAPI::Broodwar->isInGame())
        {
            BWAPI::BWAPIClient.update();
        }

        // Check to see if Starcraft shut down somehow
        if (BWAPI::BroodwarPtr == nullptr) { break; }

        // If we are successfully in a game, call the module to play the game
        if (BWAPI::Broodwar->isInGame())
        {
            if (!BWAPI::Broodwar->isReplay()) 
            { 
                std::cout << "Playing Game " << gameCount++ << " on map " << BWAPI::Broodwar->mapFileName() << "\n";
                PlayGame(); 
            }
            else 
            { 
                std::cout << "Parsing Replay " << gameCount++ << " on map " << BWAPI::Broodwar->mapFileName() << "\n";
                ParseReplay(); 
            }
        }
    }

	return 0;
}

void PlayGame()
{
    StarterBot bot;

    // The main game loop, which continues while we are connected to BWAPI and in a game
    while (BWAPI::BWAPIClient.isConnected() && BWAPI::Broodwar->isInGame())
    {
        // Handle each of the events that happened on this frame of the game
        for (const BWAPI::Event& e : BWAPI::Broodwar->getEvents())
        {
            switch (e.getType())
            {
                case BWAPI::EventType::MatchStart:   { bot.onStart();                       break; }
                case BWAPI::EventType::MatchFrame:   { bot.onFrame();                       break; }
                case BWAPI::EventType::MatchEnd:     { bot.onEnd(e.isWinner());             break; }
                case BWAPI::EventType::UnitShow:     { bot.onUnitShow(e.getUnit());         break; }
                case BWAPI::EventType::UnitHide:     { bot.onUnitHide(e.getUnit());         break; }
                case BWAPI::EventType::UnitCreate:   { bot.onUnitCreate(e.getUnit());       break; }
                case BWAPI::EventType::UnitMorph:    { bot.onUnitMorph(e.getUnit());        break; }
                case BWAPI::EventType::UnitDestroy:  { bot.onUnitDestroy(e.getUnit());      break; }
                case BWAPI::EventType::UnitRenegade: { bot.onUnitRenegade(e.getUnit());     break; }
                case BWAPI::EventType::UnitComplete: { bot.onUnitComplete(e.getUnit());     break; }
                case BWAPI::EventType::SendText:     { bot.onSendText(e.getText());         break; }
            }
        }

        BWAPI::BWAPIClient.update();
        if (!BWAPI::BWAPIClient.isConnected())
        {
            std::cout << "Disconnected\n";
            break;
        }
    }

    std::cout << "Game Over\n";
}

void ParseReplay()
{
    ReplayParser parser;

    // The main game loop, which continues while we are connected to BWAPI and in a game
    while (BWAPI::BWAPIClient.isConnected() && BWAPI::Broodwar->isInGame())
    {
        // Handle each of the events that happened on this frame of the game
        for (const BWAPI::Event& e : BWAPI::Broodwar->getEvents())
        {
            switch (e.getType())
            {
            case BWAPI::EventType::MatchStart:   { parser.onStart();                       break; }
            case BWAPI::EventType::MatchFrame:   { parser.onFrame();                       break; }
            case BWAPI::EventType::MatchEnd:     { parser.onEnd(e.isWinner());             break; }
            case BWAPI::EventType::UnitShow:     { parser.onUnitShow(e.getUnit());         break; }
            case BWAPI::EventType::UnitHide:     { parser.onUnitHide(e.getUnit());         break; }
            case BWAPI::EventType::UnitCreate:   { parser.onUnitCreate(e.getUnit());       break; }
            case BWAPI::EventType::UnitMorph:    { parser.onUnitMorph(e.getUnit());        break; }
            case BWAPI::EventType::UnitDestroy:  { parser.onUnitDestroy(e.getUnit());      break; }
            case BWAPI::EventType::UnitRenegade: { parser.onUnitRenegade(e.getUnit());     break; }
            case BWAPI::EventType::UnitComplete: { parser.onUnitComplete(e.getUnit());     break; }
            case BWAPI::EventType::SendText:     { parser.onSendText(e.getText());         break; }
            }
        }

        BWAPI::BWAPIClient.update();
        if (!BWAPI::BWAPIClient.isConnected())
        {
            std::cout << "Disconnected\n";
            break;
        }
    }
}

void updateIniFile(const std::unordered_map<std::string, std::string>& settings)
{
    namespace fs = std::filesystem;

    const std::vector<fs::path> candidatePaths = {
        fs::path("bwapi-data") / "bwapi.ini",
        fs::path("Starcraft") / "bwapi-data" / "bwapi.ini",
        fs::path("..") / "Starcraft" / "bwapi-data" / "bwapi.ini",
        fs::path("..") / ".." / "Starcraft" / "bwapi-data" / "bwapi.ini"
    };

    fs::path iniPath;
    for (const auto& candidate : candidatePaths)
    {
        if (fs::exists(candidate))
        {
            iniPath = candidate;
            break;
        }
    }

    if (iniPath.empty())
    {
        std::cerr << "Failed to locate bwapi.ini. Current path: "
                  << fs::current_path().string() << "\n";
        return;
    }

    const std::string iniFilePath = iniPath.string();

    auto trim = [](std::string value)
    {
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch)
        {
            return !std::isspace(ch);
        }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch)
        {
            return !std::isspace(ch);
        }).base(), value.end());
        return value;
    };

    std::ifstream inFile(iniFilePath);
    if (!inFile.is_open())
    {
        std::cerr << "Failed to open " << iniFilePath << " for reading\n";
        return;
    }

    std::vector<std::string> lines;
    lines.reserve(256);

    std::unordered_map<std::string, bool> updated;
    updated.reserve(settings.size());
    for (const auto& kvp : settings)
    {
        updated[kvp.first] = false;
    }

    std::string line;
    while (std::getline(inFile, line))
    {
        std::string original = line;
        std::string trimmed = trim(line);

        if (!trimmed.empty() && trimmed[0] != ';' && trimmed[0] != '[')
        {
            auto eqPos = trimmed.find('=');
            if (eqPos != std::string::npos)
            {
                std::string key = trim(trimmed.substr(0, eqPos));
                auto it = settings.find(key);
                if (it != settings.end())
                {
                    auto originalEqPos = original.find('=');
                    std::string prefix = (originalEqPos != std::string::npos)
                        ? original.substr(0, originalEqPos + 1)
                        : key + " =";
                    original = prefix + " " + it->second;
                    updated[key] = true;
                }
            }
        }

        lines.push_back(original);
    }
    inFile.close();

    for (const auto& kvp : settings)
    {
        if (!updated[kvp.first])
        {
            lines.push_back(kvp.first + " = " + kvp.second);
        }
    }

    std::ofstream outFile(iniFilePath, std::ios::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "Failed to open " << iniFilePath << " for writing\n";
        return;
    }

    for (const auto& outLine : lines)
    {
        outFile << outLine << "\n";
    }

    std::cout << "Updated ini file: " << iniFilePath << "\n";
    for (const auto& kvp : settings)
    {
        std::cout << kvp.first << " = " << kvp.second << "\n";
    }

}


