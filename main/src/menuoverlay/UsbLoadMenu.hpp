#pragma once

#include "C64Emu.hpp"
#include "MenuBaseClass.hpp"
#include "menuoverlay/MenuTypes.hpp"

class UsbLoadMenu : public MenuBaseClass {
   private:
    C64Emu*               c64emu = nullptr;
    std::vector<MenuItem> getDirPage(uint16_t page);
    void                  loadPrg(MenuItem* item);
    uint16_t              currentPage  = 0;
    uint16_t              nextPage     = 0;
    size_t                pageSize     = 12;
    bool                  needsRefresh = true;

   public:
    UsbLoadMenu(std::string title, MenuBaseClass* previousMenu, MenuController* menuController);
    ~UsbLoadMenu();

    bool init() override;
    void update() override;
    void navigateBegin() override;
    void toPrevPage();
    void toNextPage();
    void displayMenu();
};
