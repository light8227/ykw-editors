#include <iostream>
#include <cstring>
#include <dirent.h>
#include <switch.h>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <map>
#include "cipher.h"
#include "structs.h"

Result get_save(u64* application_id, AccountUid* uid) {
    Result rc = 0;
    FsSaveDataInfoReader reader;
    s64 total_entries = 0;
    FsSaveDataInfo info;
    bool found = 0;

    rc = fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);

    if (R_FAILED(rc)) {
        std::cout << "fsOpenSaveDataInfoReader() failed: 0x" << std::hex << rc << std::dec << std::endl;
        return rc;
    }

    while (true) {
        rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);

        if (R_FAILED(rc) || total_entries == 0)
            break;

        if (info.save_data_type == FsSaveDataType_Account) {
            *uid = info.uid;
            found = true;
            break;
        }
    }

    fsSaveDataInfoReaderClose(&reader);

    if (R_SUCCEEDED(rc) && !found)
        return MAKERESULT(Module_Libnx, LibnxError_NotFound);

    return rc;
}

int main(int argc, char** argv) {
    Result rc = 0;

    DIR* dir;
    struct dirent* ent;

    AccountUid uid = {0};
    u64 application_id = 0x0100c0000ceea000; // ApplicationId of the save to mount. TODO add game selection

    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    if (R_FAILED(get_save(&application_id, &uid))) {
        rc = accountInitialize(AccountServiceType_Application);

        if (R_FAILED(rc)) {
            std::cout << "accountInitialize() failed: 0x" << std::hex << rc << std::dec << std::endl;
        }

        if (R_SUCCEEDED(rc)) {
            rc = accountGetPreselectedUser(&uid);
            accountExit();

            if (R_FAILED(rc)) {
                std::cout << "accountGetPreselectedUser() failed: 0x" << std::hex << rc << std::dec << std::endl;
            }
        }
    }

    if (R_SUCCEEDED(rc)) {
        std::cout << "application_id=0x" << std::hex << application_id << " uid: 0x" << uid.uid[1] << " 0x" << uid.uid[0] << std::dec << std::endl;
    }

    if (R_SUCCEEDED(rc)) {
        rc = fsdevMountSaveData("save", application_id, uid);

        if (R_FAILED(rc)) {
            std::cout << "fsdevMountSaveData() failed: 0x" << std::hex << rc << std::dec << std::endl;
            std::cout << "Close your game and try again." << std::endl;
        }
    }

    if (R_SUCCEEDED(rc)) {
        dir = opendir("save:/");

        if (dir == NULL) {
            std::cout << "Failed to open dir." << std::endl;
        } else {
            while ((ent = readdir(dir))) {
                if (strcmp(ent->d_name, "game1.yw") == 0) { //TODO add save selection
                    char filePath[256];
                    snprintf(filePath, sizeof(filePath), "save:/%s", ent->d_name);
                    FILE* file = fopen(filePath, "r+b");

                    if (file == NULL) {
                        std::cout << "Failed to open file " << filePath << "." << std::endl;
                    } else {
                        char buffer[256];
                        size_t bytesRead;

                        std::vector<uint8_t> encryptedData;

                        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                            encryptedData.insert(encryptedData.end(), buffer, buffer + bytesRead);
                        }
                        
                        std::vector<uint8_t> decryptedData = yw_proc(encryptedData, false);

                        // printf("Decrypted data: ");
                        // int bytes = 0;
                        // for (const auto& byte : decryptedData) {
                        //     if (bytes == 120) {
                        //         break;
                        //     }
                        //     printf("%02x", byte);
                        //     bytes++;
                        // }

                        //EDIT HERE
                        
                        uint32_t* x = (uint32_t*)(&decryptedData[20]);
                        uint32_t* y = (uint32_t*)(& decryptedData[24]);
                        uint32_t* z = (uint32_t*)(& decryptedData[28]);

                        uint64_t* location = (uint64_t*)(& decryptedData[112]);

                        uint16_t* time = (uint16_t*)(& decryptedData[1752]);
                        uint8_t* sun = (uint8_t*)(& decryptedData[1754]);

                        uint32_t* money = (uint32_t*)(& decryptedData[37620]); 


                        int16_t offset;


                        std::vector<Yokai> yokailist;
                        offset = 7696;
                        for (int i = 0; i < 240; i++) {

                            if (decryptedData[offset+2] == 0) {
                                break;
                            }

                            yokailist.push_back(Yokai(decryptedData, offset));
                            offset += 124;
                        }

                        std::vector<Item> itemlist;
                        offset = 1784;
                        for (int i = 0; i < 256; i++) {

                            if (decryptedData[offset+2] == 0) {
                                break;
                            }
                            
                            itemlist.push_back(Item(decryptedData, offset));
                            offset += 12;
                        }

                        std::vector<Equipment> equipmentlist;
                        offset = 4868;
                        for (int i = 0; i < 100; i++) {

                            if (decryptedData[offset+2] == 0) {
                                break;
                            }

                            equipmentlist.push_back(Equipment(decryptedData, offset));
                            offset += 12;
                        }

                        std::vector<Important> importantlist;
                        offset = 6480;
                        for (int i = 0; i < 150; i++) {

                            if (decryptedData[offset+2] == 0) {
                                break;
                            }

                            importantlist.push_back(Important(decryptedData, offset));
                            offset += 8;
                        }

                        // std::vector<int> medalliumlist; //TODO
                        // offset = 1476;
                        // importantlist.push_back();

                        printf("\nYokai:");

                        for (int i = 0; i < yokailist.size(); i++) {
                            wprintf(L"\n%d: %u %s", i, yokailist[i].yokai, yokailist[i].nickname); //the unicode is not compatible with the terminal (could do a regular printf but it would be unreadable)
                        }

                        printf("\n\nItems:");

                        for (int i = 0; i < itemlist.size(); i++) {
                            printf("\n%d: %u", i, itemlist[i].item);
                        }

                        printf("\n\nEquipment:");

                        for (int i = 0; i < equipmentlist.size(); i++) {
                            printf("\n%d: %u", i, equipmentlist[i].equipment);
                        }

                        printf("\n\nImportant:");

                        for (int i = 0; i < importantlist.size(); i++) {
                            printf("\n%d: %u", i, importantlist[i].important);
                        }
                        
                        bool save = true;

                        printf("\n\n+ to save and exit, - to exit without saving \nA to increment money, B to set all yokai to togenyan");
                        printf("\nmoney: %u", *money);
                        while (appletMainLoop()) {
                            padUpdate(&pad);
                            u64 kDown = padGetButtonsDown(&pad);

                            if (kDown & HidNpadButton_Plus){
                                break;
                            }
                            if (kDown & HidNpadButton_Minus){
                                save = false;
                                break;
                            }
                            if (kDown & HidNpadButton_A){
                                (*money)++;
                                printf("\nmoney: %u", *money);
                            }
                            if (kDown & HidNpadButton_B){
                                for (int i = 0; i < yokailist.size(); i++) {
                                    *yokailist[i].yokai = 72463062;
                                }
                                printf("\nall yokai set to togenyan");
                            }

                            consoleUpdate(NULL);
                        }

                        
                        if (save) {
                            fseek(file, 0, SEEK_SET);
                            fwrite(yw_proc(decryptedData, true).data(), 1, 47564, file);
                            fclose(file);

                            std::string bakfilePath = filePath;
                            bakfilePath.replace(bakfilePath.rfind(".yw"), 3, ".bak");
                            FILE* bakfile = fopen(bakfilePath.c_str(), "wb");
                            fwrite(encryptedData.data(), 1, 47564, bakfile);
                            fclose(bakfile);
                            fsdevCommitDevice("save");
                        }
                        else {
                            fclose(file);
                        }
                    }
                }
            }

            closedir(dir);
            std::cout << "\nDone." << std::endl;
        }

        fsdevUnmountDevice("save");
    }

    consoleExit(NULL);
    return 0;
}