// StickC Plus 2 file browser with read-only text and image previews.
#pragma once
#include <Arduino.h>
#include <M5Unified.h>

class FileMgrMode {
public:
    static void start();
    static void stop();
    static void update();
    static void draw(M5Canvas& canvas);
    static bool isRunning() { return running; }
    static void getStatusLine(char* buf, size_t n);

private:
    enum class Phase : uint8_t {
        BROWSE, ACTIONS, VIEW, IMAGE, INFO, CONFIRM_DEL, CONFIRM_OVERWRITE
    };
    enum class ClipboardAction : uint8_t { EMPTY, COPY, MOVE };
    enum class Action : uint8_t { OPEN, COPY, MOVE, PASTE, DELETE, REFRESH, UP, EXIT };

    struct Entry {
        char name[64];
        bool isDir;
        uint32_t size;
    };

    static constexpr uint16_t PREVIEW_CAP = 6144;
    static constexpr uint16_t PATH_BUF = 192;
    static constexpr uint8_t NAME_BUF = 64;
    static constexpr uint8_t STATUS_BUF = 40;
    static constexpr uint8_t VIS_ROWS = 5;
    static constexpr uint8_t ACTION_ROWS = 5;

    static bool running;
    static Phase phase;

    static char* curPath;
    static Entry* entries;
    static uint16_t entryCount;
    static uint16_t entryCapacity;
    static bool listTruncated;
    static uint16_t sel;
    static uint16_t scroll;
    static bool keyLatch;
    static char* statusMsg;
    static Action actionItems[8];
    static uint8_t actionCount;
    static uint8_t actionSel;
    static uint8_t actionScroll;

    // Read-only preview buffer for one file at a time.
    static char* buf;
    static uint16_t bufLen;
    static char* openName;
    static uint16_t viewTopLine;
    static char* clipboardPath;
    static ClipboardAction clipboardAction;

    static bool allocateRuntimeBuffers();
    static void freeRuntimeBuffers();
    static bool allocatePreviewBuffer();
    static void freePreviewBuffer();
    static bool reserveEntries(uint16_t required);
    static void refreshList();
    static void enterDir(const char* name);
    static void goUp();
    static bool openSelected();
    static bool loadFile(const char* path);
    static void buildFullPath(char* out, size_t n, const char* name);
    static bool isTextFile(const char* name);
    static bool imageType(const char* name);
    static void buildActionList();
    static void executeAction(Action action);
    static bool pasteClipboard();
    static bool copyPath(const char* src, const char* dst, uint8_t depth = 0);
    static bool removePath(const char* path, uint8_t depth = 0);
    static void back();
    static void handleBrowseInput();
    static void handleActionsInput();
    static void handleViewInput();
    static void drawBrowse(M5Canvas& canvas);
    static void drawActions(M5Canvas& canvas);
    static void drawView(M5Canvas& canvas);
    static void drawImage(M5Canvas& canvas);
};
