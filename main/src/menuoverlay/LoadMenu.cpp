#include "LoadMenu.hpp"
#include <dirent.h>
#include <string.h>
#include <sys/_default_fcntl.h>
#include <cstddef>
#include <cstdint>
#include "C64Emu.hpp"
#include "Config.hpp"
#include "ExternalCmds.hpp"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "menuoverlay/MenuController.hpp"
#include "menuoverlay/MenuTypes.hpp"
#include "portmacro.h"

const static char* TAG = "LoadMenu";

LoadMenu::LoadMenu(std::string title, MenuBaseClass* previousMenu, MenuController* menuController)
    : MenuBaseClass(title, previousMenu, menuController)
{
    // Nothing else to do here
    c64emu = menuController->getC64Emu();
    sdcard = &c64emu->externalCmds.sdcard;
    // Initialize SD card
    // sdcard.init();
}

LoadMenu::~LoadMenu() {};

std::vector<MenuItem> LoadMenu::getDirPage(uint16_t page)
{
    ESP_LOGI(TAG, "Loading directory page %d", page);

    DIR* dir = opendir(SD_CARD_PRG_PATH);
    if (!dir) {
        MenuItem item;
        item.id     = 0;
        item.title  = "(No SD card mounted)";
        item.type   = MenuItemType::ACTION;
        item.action = [](MenuItem*) {};
        this->items.push_back(item);
        return items;
    }
    closedir(dir);

    auto     entries  = sdcard->listPagedEntries(SD_CARD_PRG_PATH, page, pageSize);
    uint16_t id_count = 0;
    for (auto& entry : entries) {
        MenuItem item = MenuItem();
        item.id       = id_count++;
        item.title    = entry;
        item.type     = MenuItemType::ACTION;
        item.action   = [this](MenuItem* item) { this->loadPrg(item); };
        this->items.push_back(item);
    }

    return items;
}

void LoadMenu::toNextPage()
{
    nextPage++;
    navigateBegin();
    ESP_LOGI(TAG, "Loading next page %d", nextPage);
}

void LoadMenu::toPrevPage()
{
    if (nextPage > 0) {
        nextPage--;
        navigateBegin();
    }
    ESP_LOGI(TAG, "Loading previous page %d", nextPage);
}

void LoadMenu::displayMenu()
{
    // Free any existing menu items
    items.clear();

    // Display the menu
    if (currentPage != 0) {
        MenuItem nextPageItem = MenuItem();
        nextPageItem.id       = 20;
        nextPageItem.title    = "=== Prev Page ===";
        nextPageItem.type     = MenuItemType::ACTION;
        nextPageItem.action   = [this](MenuItem* item) { this->toPrevPage(); };
        this->items.push_back(nextPageItem);
    }

    // Load the next page of menu items
    this->items = getDirPage(currentPage);

    // Display the menu
    MenuItem nextPageItem = MenuItem();
    nextPageItem.id       = 20;
    nextPageItem.title    = "=== Next Page ===";
    nextPageItem.type     = MenuItemType::ACTION;
    nextPageItem.action   = [this](MenuItem* item) { this->toNextPage(); };
    this->items.push_back(nextPageItem);
}

void LoadMenu::loadPrg(MenuItem* item)
{
    ExternalCmds* ext = &c64emu->externalCmds;

    // First reset the C64
    ext->reset();
    vTaskDelay(3000 / portTICK_PERIOD_MS);  // ~1second
    // Load the program
    ext->loadPrg(item->title.c_str());
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // ~1second
    menuController->hide();
}

void LoadMenu::navigateBegin()
{
    needsRefresh = true;
    nextPage     = 0;
    MenuBaseClass::navigateBegin();
}

void LoadMenu::update()
{
    ESP_LOGI(TAG, "Updating load menu");
    if (needsRefresh || currentPage != nextPage) {
        needsRefresh = false;
        currentPage  = nextPage;
        displayMenu();
    }
}

bool LoadMenu::init()
{
    sdcard->init();
    ESP_LOGI(TAG, "Initializing load menu...");
    needsRefresh = true;
    currentPage  = 0;
    nextPage     = 0;

    return true;
}
