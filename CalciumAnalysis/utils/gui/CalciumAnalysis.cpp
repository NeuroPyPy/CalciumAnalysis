#include "pch.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "CalciumAnalysis.h"

std::filesystem::path HomeDirectory;
std::filesystem::path DataDirectory;



/// <summary>Recursively gets files from a certain directory.</summary>
/// <remarks>These files can be filtered by if they end with certain file extensions.</remarks>
/// <param name="directory">Path to the directory to get the files from</param>
/// <param name="extensions">List of file extensions to filter by</param>
/// <param name="depth">Current folder depth</param>
/// <param name="maxDepth">Max folder depth to iterate through</param>
/// <returns>The files from a certain directory</returns>
std::vector<std::filesystem::path> InstantSuite::IterateDirectory(const std::filesystem::path& directory,
	const std::vector<std::string>& extensions,
	const int depth, const int maxDepth)
{
	if (depth > maxDepth) {
		return std::vector<std::filesystem::path>();
	}

	std::vector<std::filesystem::path> files;
	for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator(directory)) {
		const std::filesystem::path& filePath = file.path();
		if (file.is_directory()) {
			std::vector<std::filesystem::path> directoryFiles = IterateDirectory(
				filePath, extensions, depth + 1, maxDepth);
			// Remove if directory is empty.
			if (!directoryFiles.empty()) {
				files.insert(files.end(), directoryFiles.begin(), directoryFiles.end());
			}
		}
		else if (HasExtension(filePath.extension().string(), extensions)) {
			files.push_back(filePath);
		}
	}
	return files;
}


/// <summary>Gets files from a certain directory.</summary>
/// <remarks>These files can be filtered by if they end with certain file extensions.</remarks>
/// <param name="directory">Path to the directory to get the files from</param>
/// <param name="numExtension">Number if filters to filter the files by</param>
/// <param name="...">Multiple file extensions to filter by</param>
/// <returns>The files from a certain directory</returns>
std::vector<std::filesystem::path> InstantSuite::GetFilesFromDir(const std::filesystem::path& directory,
	int numExtension, ...)
{
	if (!exists(directory)) {
		return std::vector<std::filesystem::path>();
	}

	va_list extensions;
	std::vector<std::string> fileExtensions;

	va_start(extensions, numExtension);
	for (int i = 0; i < numExtension; i++) {
		fileExtensions.emplace_back(va_arg(extensions, const char*));
	}
	va_end(extensions);

	return IterateDirectory(directory, fileExtensions, 0, 1);
}

/// <summary>Gets workshop maps from the given directory.</summary>
/// <param name="workshopPath">Path to the workshop directory to get the maps from</param>
/// <param name="extensions">List of map extensions to filter by</param>
/// <param name="preferredExtension">Map extension to prefer when multiple files are found</param>
/// <returns>The workshop maps from the given directory</returns>
std::vector<std::filesystem::path> InstantSuite::getWorkshopMaps(const std::filesystem::path& workshopPath,
	const std::vector<std::string>& extensions,
	const std::string& preferredExtension)
{
	if (!exists(workshopPath)) {
		return std::vector<std::filesystem::path>();
	}

	// Make sure we don't request workshop map names every tick.
	const bool shouldRequestWorkshopMapNames = publishedFileID.empty();
	std::vector<std::filesystem::path> files = IterateDirectory(workshopPath, extensions, 0, 1);
	std::filesystem::path bestPath;
	std::vector<std::filesystem::path> workshopMaps;
	for (const std::filesystem::path& file : files) {
		if (file.parent_path() != bestPath.parent_path()) {
			if (!bestPath.empty()) {
				const uint64_t workshopMapId = std::strtoull(bestPath.parent_path().stem().string().c_str(), nullptr,
					10);
				if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.end()) {
					publishedFileID.push_back(workshopMapId);
				}
				workshopMaps.push_back(bestPath);
			}
			bestPath = file;
		}
		else if (bestPath.extension() != preferredExtension && file.extension() == preferredExtension) {
			bestPath = file;
		}
	}

	if (!bestPath.empty()) {
		const uint64_t workshopMapId = std::strtoull(bestPath.parent_path().stem().string().c_str(), nullptr, 10);
		if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.end()) {
			publishedFileID.push_back(workshopMapId);
		}
		workshopMaps.push_back(bestPath);
	}


	return workshopMaps;
}

/*
 *  Main Plugin Functions
 */

void InstantSuite::setPluginEnabled(bool newPluginEnabled)
{
	cvarManager->getCvar(enabledCvarName).setValue(newPluginEnabled);
	cvarManager->log("ps. Plugin Enabled Cvar has been set");
}

void InstantSuite::pluginEnabledChanged()
{
	const bool enabled = cvarManager->getCvar(enabledCvarName).getBoolValue();

	if (enabled)
	{
		if (!hooked)
		{
			hookMatchEnded();
			cvarManager->log("ps. Hooked match ended (PluginEnabledChanged Function)");
		}
	}
	else
	{
		if (hooked)
		{
			unhookMatchEnded();
			cvarManager->log("Unhooked match ended (PluginEnabledChanged Function");
		}
	}
}

void InstantSuite::setDelay(float newDelay)
{
	if (newDelay == 0) {
		return;
	}
	else {
		cvarManager->getCvar(DelayCvarName).setValue(newDelay);
		cvarManager->log("ps. DelayCvarName has been set");
	}
}

void InstantSuite::setDisableCasual(bool newDisCasual)
{
	cvarManager->getCvar(disableCasualCvarName).setValue(newDisCasual);
	cvarManager->log("ps. disableCasualCvarName has been set");
}

void InstantSuite::setDisablePrivate(bool newDisPrivate)
{

	cvarManager->getCvar(disablePrivateCvarName).setValue(newDisPrivate);
	cvarManager->log("ps. disablePrivateCvarName has been set");

}


/*
 *  Instant Queue Functions
 */

void InstantSuite::queue(ServerWrapper server, void* params, std::string eventName)
{

	float totalQueueDelayTime = 0;
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();
	float queueDelayTime = cvarManager->getCvar(qDelayCvarName).getFloatValue();
	bool disableCasualQueue = cvarManager->getCvar(disableCasualQCvarName).getBoolValue();
	bool disablePrivate = cvarManager->getCvar(disablePrivateCvarName).getBoolValue();

	if (autoGG) {
		totalQueueDelayTime = queueDelayTime + autoGGDelayTime;
		cvarManager->log("ps. totalQueueDelayTime has been set");
	}
	else {
		totalQueueDelayTime = queueDelayTime;
	}

	if (!server.IsNull() && (disablePrivate || disableCasualQueue))
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if ((playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) && disableCasualQueue) {
			return;
		}
		else if ((playlist == Private || playlist == Tournament) && disablePrivate) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedQueue, this), queueDelayTime);
			cvarManager->log("ps. settimeout(delayedQueue, queueDelaytime) has been called");

		}
	}

	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedQueue, this), queueDelayTime);
	cvarManager->log("ps. settimeout(delayedQueue, queueDelaytime) has been called");
}

void InstantSuite::setQueueEnabled(bool newQueueEnabled)
{
	cvarManager->getCvar(queueCvarName).setValue(newQueueEnabled);
	cvarManager->log("ps. queueCvarName has been set");
}

void InstantSuite::setDelayedQueue(float newQueueDelayTime)
{
	if (newQueueDelayTime == 0) {
		return;
	}
	else {
		cvarManager->getCvar(qDelayCvarName).setValue(newQueueDelayTime);
		cvarManager->log("ps. qDelayCvarName has been set");
	}
}

void InstantSuite::delayedQueue()
{
	auto game = gameWrapper->GetOnlineGame();
	if (game.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("queue");
	cvarManager->log("InstantSuite queue command executed");
}

void InstantSuite::setDisableCasualQueue(bool newDisCasualQueue)
{
	cvarManager->getCvar(disableCasualQCvarName).setValue(newDisCasualQueue);
	cvarManager->log("ps. disableCasualQCvarName has been set");
}


/*
 *  Instant Freeplay Functions
 */

void InstantSuite::launchTraining(ServerWrapper server, void* params, std::string eventName)
{
	float totalTrainingDelayTime = 0;
	float trainingDelayTime = cvarManager->getCvar(DelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();
	if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (autoGG) {
		totalTrainingDelayTime = trainingDelayTime + autoGGDelayTime;
		cvarManager->log("ps. training delay time has been set");
	}
	else {
		totalTrainingDelayTime = trainingDelayTime;
		cvarManager->log("ps. training delay time has been set");
	}

	bool disableCasualTraining = cvarManager->getCvar(disableCasualCvarName).getBoolValue();
	bool disablePrivate = cvarManager->getCvar(disablePrivateCvarName).getBoolValue();

	if (!server.IsNull() && (server.GetPlaylist().memory_address != NULL) && (disablePrivate || disableCasualTraining))
		// ! reverses the result, && returns if both statements true, || returns if one statement is true
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if ((playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) && disableCasualTraining) {
			return;
		}
		else if ((playlist == Private || playlist == Tournament || playlist == GodBall || playlist == GodBallDoubles) && disablePrivate) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedTraining, this), totalTrainingDelayTime);
			cvarManager->log("ps. settimeout(delayedTraining, totalTrainingDelayTime executed");
		}
	}

	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedTraining, this), totalTrainingDelayTime);
	cvarManager->log("ps. settimeout(delayedTraining, totalTrainingDelayTime executed");
}

void InstantSuite::setTrainingEnabled(bool newTrainingEnabled)
{
	cvarManager->getCvar(trainingCvarName).setValue(newTrainingEnabled);
	cvarManager->log("ps. trainingCvarName has been set");
}

void InstantSuite::delayedTraining()
{
	std::stringstream launchTrainingCommandBuilder;
	std::string mapname = cvarManager->getCvar(trainingMapCvarName).getStringValue();

	if (mapname.compare("random") == 0)
	{
		mapname = gameWrapper->GetRandomMap();
		cvarManager->log("ps. Random map aquired. ");
	}

	launchTrainingCommandBuilder << "start " << mapname << "?Game=TAGame.GameInfo_Tutorial_TA?GameTags=Training";
	const std::string launchTrainingCommand = launchTrainingCommandBuilder.str();
	auto game = gameWrapper->GetOnlineGame();
	if (game.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("load_freeplay");
	//gameWrapper->ExecuteUnrealCommand(launchTrainingCommand);
	cvarManager->log("InstantSuite launchTrainingCommand executed.");

}

/*
 *  Instant Custom Training Functions
 */

void InstantSuite::launchCustomTraining(ServerWrapper server, void* params, std::string eventName)
{
	float totalCustomTrainingDelayTime = 0;
	float CustomTrainingDelayTime = cvarManager->getCvar(DelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();
	if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (autoGG) {
		totalCustomTrainingDelayTime = CustomTrainingDelayTime + autoGGDelayTime;
	}
	else {
		totalCustomTrainingDelayTime = CustomTrainingDelayTime;
	}

	bool disableCasualTraining = cvarManager->getCvar(disableCasualCvarName).getBoolValue();
	bool disablePrivate = cvarManager->getCvar(disablePrivateCvarName).getBoolValue();

	if (!server.IsNull() && (server.GetPlaylist().memory_address != NULL) && (disablePrivate || disableCasualTraining))
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if ((playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) && disableCasualTraining) {
			return;
		}
		else if ((playlist == Private || playlist == Tournament || playlist == GodBall || playlist == GodBallDoubles) && disablePrivate) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedCustomTraining, this), totalCustomTrainingDelayTime);
		}
	}

	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedCustomTraining, this), totalCustomTrainingDelayTime);
}

void InstantSuite::setCustomTrainingEnabled(bool newCustomTrainingEnabled)
{
	cvarManager->getCvar(ctrainingCvarName).setValue(newCustomTrainingEnabled);
}

void InstantSuite::setCustomTrainingCode(std::string newCustomTrainingCode)
{
	cvarManager->getCvar(customtrainingCvarName).setValue(newCustomTrainingCode);
}

void InstantSuite::delayedCustomTraining()
{
	auto game = gameWrapper->GetOnlineGame();
	auto training_code = cvarManager->getCvar(customtrainingCvarName).getStringValue();

	if (game.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("load_training " + training_code);
}


/*
 *  Instant Workshop Functions (Working, not integrated to ImGui yet)
 */

void InstantSuite::launchWorkshop(ServerWrapper server, void* params, std::string eventName)
{
	float totalWorkshopDelayTime = 0;
	float WorkshopDelayTime = cvarManager->getCvar(DelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();
	if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (autoGG) {
		totalWorkshopDelayTime = WorkshopDelayTime + autoGGDelayTime;
	}
	else {
		totalWorkshopDelayTime = WorkshopDelayTime;
	}

	bool disableCasualTraining = cvarManager->getCvar(disableCasualCvarName).getBoolValue();
	bool disablePrivate = cvarManager->getCvar(disablePrivateCvarName).getBoolValue();

	if (!server.IsNull() && (server.GetPlaylist().memory_address != NULL) && (disablePrivate || disableCasualTraining))
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if ((playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) && disableCasualTraining) {
			return;
		}
		else if ((playlist == Private || playlist == Tournament || playlist == GodBall || playlist == GodBallDoubles) && disablePrivate) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedWorkshop, this), totalWorkshopDelayTime);
		}
	}

	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedWorkshop, this), totalWorkshopDelayTime);
}

void InstantSuite::setWorkshopEnabled(bool newWorkshopEnabled)
{
	cvarManager->getCvar(wtrainingCvarName).setValue(newWorkshopEnabled);
}

void InstantSuite::delayedWorkshop()
{
	auto game = gameWrapper->GetOnlineGame();
	std::string workshop_map = cvarManager->getCvar(workshopCvarName).getStringValue();

	if (game.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("load_workshop " + workshop_map);
	//cvarManager->executeCommand("load_workshop " + currentMap);
}


/*
 *  Instant Exit Functions
 */

void InstantSuite::exitGame(ServerWrapper server, void* params, std::string eventName)
{
	float totalExitDelayTime = 0;
	float exitDelayTime = cvarManager->getCvar(DelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();
	if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (autoGG) {
		totalExitDelayTime = exitDelayTime + autoGGDelayTime;
	}
	else {
		totalExitDelayTime = exitDelayTime;
	}

	bool disableCasualExit = cvarManager->getCvar(disableCasualCvarName).getBoolValue();
	bool disablePrivate = cvarManager->getCvar(disablePrivateCvarName).getBoolValue();

	if (!server.IsNull() && (disablePrivate || disableCasualExit))
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if ((playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) && disableCasualExit) {
			return;
		}
		else if ((playlist == Private || playlist == Tournament) && disablePrivate) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedExit, this), totalExitDelayTime);
		}
	}

	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedExit, this), totalExitDelayTime);
}

void InstantSuite::setExitEnabled(bool newExitEnabled)
{
	cvarManager->getCvar(exitCvarName).setValue(newExitEnabled);
}

void InstantSuite::delayedExit()
{
	auto game = gameWrapper->GetOnlineGame();
	if (game.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("unreal_command disconnect");
}

/*
 *  Keybind Functions
 */

/// <summary>
/// Change shortcut keybind: open Instant Suite GUI
/// </summary>
void InstantSuite::unbindKeybind()
{
	cvarManager->executeCommand("unbind \"" + cvarManager->getCvar("key_holder").getStringValue());
	cvarManager->log("Unbound key: \"" + cvarManager->getCvar("key_holder").getStringValue() + "\"");
}

/// <summary>
/// Change shortcut keybind: unbind desired key
/// </summary>
void InstantSuite::changeGuiKeybind()
{
	cvarManager->executeCommand("bind \"" + cvarManager->getCvar("is_gui_keybind").getStringValue() + "\" \"togglemenu instantsuite\"", true);
	cvarManager->log("Changed the keybind for \"togglemenu instantsuite\" to \"" + cvarManager->getCvar("is_gui_keybind").getStringValue() + "\"");
}


/// <summary>
/// Quick-change if plugin is enabled, sets cvar to opposite it's current value 
/// </summary>
void InstantSuite::quickPluginEnabled()
{
	bool pluginEnabled = cvarManager->getCvar(enabledCvarName).getBoolValue();

	if (pluginEnabled == false) {
		cvarManager->getCvar(enabledCvarName).setValue(true);
	}

	if (pluginEnabled == true) {
		cvarManager->getCvar(enabledCvarName).setValue(false);
	}
}

/// <summary>Checks if the GUI window is bound.</summary>
/// <param name="windowName">Name of the GUI window</param>
/// <returns>Bool with if the GUI window is bound</returns>
bool IsGUIWindowBound(const std::string& windowName)
{
	const std::string bind = "togglemenu " + windowName;
	std::ifstream file(BINDS_FILE_PATH);
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			if (line.find(bind) != std::string::npos) {
				file.close();
				return true;
			}
		}
		file.close();
	}

	return false;
}


/*
 * Server Functions 
 */

void InstantSuite::onMatchEnd(ServerWrapper server, void* params, std::string eventName)
{
	const bool exitEnabled = cvarManager->getCvar(exitCvarName).getBoolValue();
	const bool queueEnabled = cvarManager->getCvar(queueCvarName).getBoolValue();
	const bool trainingEnabled = cvarManager->getCvar(trainingCvarName).getBoolValue();
	const bool customtrainingEnabled = cvarManager->getCvar(ctrainingCvarName).getBoolValue();
	const bool workshopEnabled = cvarManager->getCvar(wtrainingCvarName).getBoolValue();
	if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck

	if (queueEnabled) {
		queue(server, params, eventName);
		if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck
	}
	if (exitEnabled) {
		exitGame(server, params, eventName);
		if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck
	}
	else {
		if (trainingEnabled) {
			launchTraining(server, params, eventName);
			if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck
		}
		else
			if (customtrainingEnabled) {
				launchCustomTraining(server, params, eventName);
				if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck
			}
			else
				if (workshopEnabled) {
					launchWorkshop(server, params, eventName);
					if (server.IsNull()) { cvarManager->log("null_pntr"); return; } //nullcheck
				}
	}
}

void InstantSuite::hookMatchEnded()
{
	gameWrapper->HookEventWithCaller<ServerWrapper>(matchEndedEvent, std::bind(&InstantSuite::onMatchEnd, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	hooked = true;
	logHookType("Hooked");
}

void InstantSuite::unhookMatchEnded()
{
	gameWrapper->UnhookEvent(matchEndedEvent);
	hooked = false;
	logHookType("Unhooked");
}

void InstantSuite::logHookType(const char* const hookType) const
{
	std::stringstream logBuffer;
	logBuffer << hookType << " match ended event.";
	cvarManager->log(logBuffer.str());
}

void InstantSuite::removeOldPlugin() { // disable deprecated predecessor plugin InstantTraining
	cvarManager->executeCommand("unload instanttraining");
	cvarManager->executeCommand("unload instantexit");
	cvarManager->executeCommand("writeplugins");
}

void InstantSuite::onLoad()
{

	BakkesModConfigFolder = gameWrapper->GetBakkesModPath() / L"cfg";
	BakkesModCrashesFolder = gameWrapper->GetBakkesModPath() / L"crashes";
	if (!exists(BakkesModCrashesFolder)) {
		std::filesystem::create_directory(BakkesModCrashesFolder);
	}
	InstantSuiteDataFolder = gameWrapper->GetDataFolder() / L"InstantSuite";
	if (!exists(InstantSuiteDataFolder)) {
		std::filesystem::create_directory(InstantSuiteDataFolder);
	}
	RocketLeagueExecutableFolder = std::filesystem::current_path();

	//Main Plugin Cvars
	cvarManager->log("Plugin loaded!");
	cvarManager->registerCvar(enabledCvarName, "1", "Determines whether Instant Suite is enabled.").addOnValueChanged(std::bind(&InstantSuite::pluginEnabledChanged, this));
	cvarManager->registerCvar(disablePrivateCvarName, "0", "Disable plugin during Private, Tournament, and Heatseeker matches.");
	cvarManager->registerCvar(DelayCvarName, "0", "Seconds to wait before loading into training mode.");

	//Training Cvars
	cvarManager->registerCvar(trainingCvarName, "0", "");
	cvarManager->registerCvar(trainingMapCvarName, "Beckwith Park (Stormy)", "Determines the map that will launch for training.");
	cvarManager->registerCvar(disableCasualCvarName, "0", "Don't automatically go to training when ending a casual game.");

	//BETA CustomTraining cvars
	cvarManager->registerCvar(ctrainingCvarName, "0", "Instantly jump into custom training at end of match.");
	cvarManager->registerCvar(customtrainingCvarName, "A0FE-F860-967D-E628", "User inputs training-pack code.");

	//BETA Workshop cvars
	cvarManager->registerCvar(wtrainingCvarName, "0", "Instantly jump into a workshop map at end of match.");
	cvarManager->registerCvar(workshopCvarName, "", "Desired workshop map.");
	workshopMapDirPath = std::make_shared<std::string>();
	cvarManager->registerCvar("is_workshop_path", WORKSHOP_MAPS_PATH.string(),
		"Default path for your workshop maps directory").bindTo(workshopMapDirPath);

	customMapDirPath = std::make_shared<std::string>();
	cvarManager->registerCvar("is_custom_path", CUSTOM_MAPS_PATH.string(),
		"Default path for your custom maps directory").bindTo(customMapDirPath);

	//Exit Cvars
	cvarManager->registerCvar(exitCvarName, "0", "Instantly exit to main menu instead of training at end of match.");

	//Queue cvars
	cvarManager->registerCvar(queueCvarName, "0", "Instantly queue for previously selected playlists at end of match.");
	cvarManager->registerCvar(qDelayCvarName, "0", "Seconds to wait before starting queue.");
	cvarManager->registerCvar(disableCasualQCvarName, "0", "Don't automatically queue when ending a casual game.");

	//Keybind Cvars
	cvarManager->registerCvar(keybindCvarName, DEFAULT_GUI_KEYBIND, "Keybind for the gui");
	cvarManager->registerCvar(keyholderCvarName, "", "Keybind for the gui");


	cvarManager->loadCfg("bakkesmod/cfg/InstantSuite");

	cvarManager->registerNotifier("is_change_keybind", [this](const std::vector<std::string>& arguments) 
		{
		std::string key;
		if (arguments.size() >= 2) {
			key = arguments[1];
		}
		else {
			key = cvarManager->getCvar("is_gui_keybind").getStringValue();
		}

		const std::string command = "togglemenu " + GetMenuName();
		cvarManager->setBind(key, command);
		}, "Adds a keybind for " + quote("togglemenu " + GetMenuName()) + " as $is_gui_keybind.",
			PERMISSION_ALL);
		
	// Set the window bind to the default keybind if is not set.
	if (!IsGUIWindowBound(GetMenuName())) {
		cvarManager->setBind(DEFAULT_GUI_KEYBIND, "togglemenu " + GetMenuName());
		
	}

	gameWrapper->SetTimeout([this](GameWrapper* gw) {
		this->removeOldPlugin();
		}, 10);

	hookMatchEnded();
}

void InstantSuite::onUnload()
{
	{
		//* Save all cvars to 'config.cfg'.
		cvarManager->backupCfg(CONFIG_FILE_PATH.string());
	}



}
