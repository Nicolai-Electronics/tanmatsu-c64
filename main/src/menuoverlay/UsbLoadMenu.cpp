#include "UsbLoadMenu.hpp"
#include <dirent.h>
#include <string>
#include "C64Emu.hpp"
#include "ExternalCmds.hpp"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "menuoverlay/MenuController.hpp"
#include "menuoverlay/MenuTypes.hpp"
#include "portmacro.h"

static const char* TAG = "UsbLoadMenu";

#define USB_PRG_PATH "/usb"

UsbLoadMenu::UsbLoadMenu(std::string title, MenuBaseClass* previousMenu, MenuController* menuController)
    : MenuBaseClass(title, previousMenu, menuController)
{
    c64emu = menuController->getC64Emu();
}

UsbLoadMenu::~UsbLoadMenu() {}

std::vector<MenuItem> UsbLoadMenu::getDirPage(uint16_t page) {
    size_t     skip    = (size_t)page * pageSize;
    size_t     skipped = 0;
    size_t     added   = 0;
    uint16_t   id_count = 0;

    DIR* dir = opendir(USB_PRG_PATH);
    if (!dir) {
        MenuItem item;
        item.id     = id_count++;
        item.title  = "(No USB disk mounted)";
        item.type   = MenuItemType::ACTION;
        item.action = [](MenuItem*) {};
        this->items.push_back(item);
        return items;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr && added < pageSize) {
        std::string name = ent->d_name;
        if (name.length() <= 4 || name.substr(name.length() - 4) != ".prg") {
            continue;
        }
        if (skipped < skip) {
            skipped++;
            continue;
        }
        MenuItem item;
        item.id     = id_count++;
        item.title  = name.substr(0, name.length() - 4);
        item.type   = MenuItemType::ACTION;
        item.action = [this](MenuItem* item) { this->loadPrg(item); };
        this->items.push_back(item);
        added++;
    }
    closedir(dir);
    return items;
}

void UsbLoadMenu::navigateBegin() {
    needsRefresh = true;
    MenuBaseClass::navigateBegin();
}

void UsbLoadMenu::toNextPage() {
    nextPage++;
    navigateBegin();
    ESP_LOGI(TAG, "Loading next page %d", nextPage);
}

void UsbLoadMenu::toPrevPage() {
    if (nextPage > 0) {
        nextPage--;
        navigateBegin();
    }
    ESP_LOGI(TAG, "Loading previous page %d", nextPage);
}

void UsbLoadMenu::displayMenu() {
    items.clear();

    if (currentPage != 0) {
        MenuItem prevPageItem;
        prevPageItem.id     = 20;
        prevPageItem.title  = "=== Prev Page ===";
        prevPageItem.type   = MenuItemType::ACTION;
        prevPageItem.action = [this](MenuItem* item) { this->toPrevPage(); };
        this->items.push_back(prevPageItem);
    }

    this->items = getDirPage(currentPage);

    MenuItem nextPageItem;
    nextPageItem.id     = 21;
    nextPageItem.title  = "=== Next Page ===";
    nextPageItem.type   = MenuItemType::ACTION;
    nextPageItem.action = [this](MenuItem* item) { this->toNextPage(); };
    this->items.push_back(nextPageItem);
}

void UsbLoadMenu::loadPrg(MenuItem* item) {
    ExternalCmds* ext = &c64emu->externalCmds;
    ext->reset();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    std::string path = std::string(USB_PRG_PATH "/") + item->title + ".prg";
    ext->loadPrgFromPath(path.c_str());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    menuController->hide();
}

void UsbLoadMenu::update() {
    if (needsRefresh || currentPage != nextPage) {
        needsRefresh = false;
        currentPage  = nextPage;
        displayMenu();
    }
}

bool UsbLoadMenu::init() {
    needsRefresh = true;
    currentPage  = 0;
    nextPage     = 0;
    return true;
}
