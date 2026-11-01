#include "filemgr.h"
#include "../storage/littlefs_ops.h"
#include "../ui/display.h"
#include "../ui/keys.h"
#include "../audio/sfx.h"
#include "../piglet/avatar.h"
#include "../core/config.h"
#include "../core/app.h"
#include <M5Unified.h>
#include <LittleFS.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
char* s_runtimeStorage = nullptr;
}

bool FileMgrMode::running = false;
FileMgrMode::Phase FileMgrMode::phase = FileMgrMode::Phase::BROWSE;
char* FileMgrMode::curPath = nullptr;
FileMgrMode::Entry* FileMgrMode::entries = nullptr;
uint16_t FileMgrMode::entryCount = 0;
uint16_t FileMgrMode::entryCapacity = 0;
bool FileMgrMode::listTruncated = false;
uint16_t FileMgrMode::sel = 0;
uint16_t FileMgrMode::scroll = 0;
bool FileMgrMode::keyLatch = false;
char* FileMgrMode::statusMsg = nullptr;
FileMgrMode::Action FileMgrMode::actionItems[8] = {};
uint8_t FileMgrMode::actionCount = 0;
uint8_t FileMgrMode::actionSel = 0;
uint8_t FileMgrMode::actionScroll = 0;
char* FileMgrMode::buf = nullptr;
uint16_t FileMgrMode::bufLen = 0;
char* FileMgrMode::openName = nullptr;
uint16_t FileMgrMode::viewTopLine = 0;
char* FileMgrMode::clipboardPath = nullptr;
FileMgrMode::ClipboardAction FileMgrMode::clipboardAction =
    FileMgrMode::ClipboardAction::EMPTY;

namespace {
static constexpr uint8_t MAX_PATH_DEPTH = 8;
static constexpr uint16_t COPY_CHUNK = 512;

class SDImageData : public lgfx::DataWrapper {
public:
    SDImageData() { need_transaction = true; }
    bool open(const char* path) override {
        file = LittleFS.open(path, FILE_READ);
        return (bool)file;
    }
    int read(uint8_t* data, uint32_t length) override {
        return file.read(data, length);
    }
    void skip(int32_t offset) override { file.seek(offset, fs::SeekCur); }
    bool seek(uint32_t offset) override { return file.seek(offset, fs::SeekSet); }
    void close() override { file.close(); }
    int32_t tell() override { return (int32_t)file.position(); }

private:
    File file;
};

static const char* baseName(const char* path) {
    const char* slash = strrchr(path ? path : "", '/');
    return slash ? slash + 1 : (path ? path : "");
}

static void formatSize(char* out, size_t n, uint32_t bytes) {
    if (bytes < 1024) snprintf(out, n, "%lu B", (unsigned long)bytes);
    else if (bytes < 1024UL * 1024UL)
        snprintf(out, n, "%lu KB", (unsigned long)((bytes + 512) / 1024));
    else snprintf(out, n, "%lu.%lu MB",
                  (unsigned long)(bytes / (1024UL * 1024UL)),
                  (unsigned long)((bytes % (1024UL * 1024UL) * 10) /
                                  (1024UL * 1024UL)));
}

static bool hasExtension(const char* name, const char* ext) {
    const char* dot = strrchr(name, '.');
    return dot && strcasecmp(dot, ext) == 0;
}

}

bool FileMgrMode::allocateRuntimeBuffers() {
    if (s_runtimeStorage) return true;
    constexpr size_t storageSize = (size_t)PATH_BUF * 2 + STATUS_BUF + NAME_BUF;
    s_runtimeStorage = static_cast<char*>(calloc(storageSize, 1));
    if (!s_runtimeStorage) return false;

    char* next = s_runtimeStorage;
    curPath = next;
    next += PATH_BUF;
    statusMsg = next;
    next += STATUS_BUF;
    openName = next;
    next += NAME_BUF;
    clipboardPath = next;
    buf = nullptr;
    strcpy(curPath, "/");
    return true;
}

void FileMgrMode::freeRuntimeBuffers() {
    freePreviewBuffer();
    free(s_runtimeStorage);
    s_runtimeStorage = nullptr;
    free(entries);
    curPath = statusMsg = buf = openName = clipboardPath = nullptr;
    entries = nullptr;
    entryCount = entryCapacity = 0;
    listTruncated = false;
}

bool FileMgrMode::allocatePreviewBuffer() {
    if (buf) return true;
    buf = static_cast<char*>(malloc(PREVIEW_CAP));
    if (!buf) return false;
    buf[0] = '\0';
    return true;
}

void FileMgrMode::freePreviewBuffer() {
    free(buf);
    buf = nullptr;
    bufLen = viewTopLine = 0;
}

bool FileMgrMode::reserveEntries(uint16_t required) {
    if (required <= entryCapacity) return true;
    size_t capacity = entryCapacity ? (size_t)entryCapacity + 32 : 32;
    if (capacity < required) capacity = required;
    if (capacity > UINT16_MAX) capacity = UINT16_MAX;
    if (capacity < required) return false;

    Entry* resized = static_cast<Entry*>(
        realloc(entries, capacity * sizeof(Entry)));
    if (!resized) return false;
    entries = resized;
    entryCapacity = (uint16_t)capacity;
    return true;
}

void FileMgrMode::buildFullPath(char* out, size_t n, const char* name) {
    if (strcmp(curPath, "/") == 0) snprintf(out, n, "/%s", name);
    else snprintf(out, n, "%s/%s", curPath, name);
}

void FileMgrMode::refreshList() {
    if (!curPath || (!entries && entryCapacity)) return;
    char keepName[NAME_BUF] = "";
    if (sel < entryCount) {
        strncpy(keepName, entries[sel].name, sizeof(keepName) - 1);
        keepName[sizeof(keepName) - 1] = '\0';
    }
    entryCount = sel = scroll = 0;
    listTruncated = false;
    if (!Storage::available() || !Config::isSDAvailable()) return;

    File dir = LittleFS.open(curPath);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return;
    }

    bool truncated = false;
    File entry = dir.openNextFile();
    while (entry) {
        const char* name = baseName(entry.name());
        if (name[0] && strcmp(name, ".") && strcmp(name, "..")) {
            if (entryCount == UINT16_MAX ||
                !reserveEntries((uint16_t)(entryCount + 1))) {
                truncated = true;
                entry.close();
                break;
            }
            Entry& dst = entries[entryCount++];
            strncpy(dst.name, name, sizeof(dst.name) - 1);
            dst.name[sizeof(dst.name) - 1] = '\0';
            dst.isDir = entry.isDirectory();
            dst.size = dst.isDir ? 0 : (uint32_t)entry.size();
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    if (entryCount > 1) {
        qsort(entries, entryCount, sizeof(Entry),
              [](const void* lhs, const void* rhs) {
                  const Entry* a = static_cast<const Entry*>(lhs);
                  const Entry* b = static_cast<const Entry*>(rhs);
                  if (a->isDir != b->isDir) return a->isDir ? -1 : 1;
                  return strcasecmp(a->name, b->name);
              });
    }
    for (uint16_t i = 0; i < entryCount; ++i) {
        if (keepName[0] && !strcmp(entries[i].name, keepName)) {
            sel = i;
            break;
        }
    }
    if (sel >= VIS_ROWS) scroll = (uint16_t)(sel - VIS_ROWS + 1);
    listTruncated = truncated;
    if (listTruncated) Display::showToast("LIST LIMITED: LOW MEM", 1400);
}

void FileMgrMode::enterDir(const char* name) {
    char next[PATH_BUF];
    buildFullPath(next, sizeof(next), name);
    if (strlen(next) >= PATH_BUF) {
        Display::showToast("PATH TOO LONG", 900);
        return;
    }
    strncpy(curPath, next, PATH_BUF - 1);
    curPath[PATH_BUF - 1] = '\0';
    refreshList();
}

void FileMgrMode::goUp() {
    if (!strcmp(curPath, "/")) return;
    char* slash = strrchr(curPath, '/');
    if (slash == curPath) curPath[1] = '\0';
    else if (slash) *slash = '\0';
    refreshList();
}

bool FileMgrMode::isTextFile(const char* name) {
    static const char* const ext[] = {
        ".txt", ".log", ".csv", ".ini", ".md", ".json", ".xml", ".html",
        ".htm", ".css", ".js", ".c", ".cpp", ".h", ".hpp", ".py", ".yaml",
        ".yml", ".conf", ".cfg", ".sh", ".toml", ".properties"
    };
    for (const char* suffix : ext) if (hasExtension(name, suffix)) return true;
    return false;
}

bool FileMgrMode::imageType(const char* name) {
    return hasExtension(name, ".jpg") || hasExtension(name, ".jpeg") ||
           hasExtension(name, ".bmp") || hasExtension(name, ".png");
}

bool FileMgrMode::loadFile(const char* path) {
    if (!allocatePreviewBuffer()) return false;
    File file = LittleFS.open(path, FILE_READ);
    if (!file || file.isDirectory()) {
        if (file) file.close();
        freePreviewBuffer();
        return false;
    }
    size_t size = file.size();
    if (size > PREVIEW_CAP - 1) size = PREVIEW_CAP - 1;
    bufLen = (uint16_t)file.read((uint8_t*)buf, size);
    file.close();
    for (uint16_t i = 0; i < bufLen; ++i) {
        uint8_t ch = (uint8_t)buf[i];
        if (ch && ch < 32 && ch != '\n' && ch != '\r' && ch != '\t') buf[i] = '.';
    }
    buf[bufLen] = '\0';
    viewTopLine = 0;
    return true;
}

bool FileMgrMode::openSelected() {
    if (!entryCount || sel >= entryCount) return false;
    Entry& entry = entries[sel];
    if (entry.isDir) {
        enterDir(entry.name);
        return true;
    }
    strncpy(openName, entry.name, NAME_BUF - 1);
    openName[NAME_BUF - 1] = '\0';
    if (imageType(entry.name)) {
        phase = Phase::IMAGE;
        return true;
    }
    if (!isTextFile(entry.name)) {
        phase = Phase::INFO;
        return true;
    }
    char path[PATH_BUF];
    buildFullPath(path, sizeof(path), entry.name);
    if (!allocatePreviewBuffer()) {
        Display::showToast("LOW MEM - TEXT", 1200);
        return false;
    }
    if (!loadFile(path)) {
        Display::showToast("OPEN FAILED", 900);
        return false;
    }
    phase = Phase::VIEW;
    return true;
}

bool FileMgrMode::removePath(const char* path, uint8_t depth) {
    if (depth > MAX_PATH_DEPTH) return false;
    File node = LittleFS.open(path);
    if (!node) return false;
    if (!node.isDirectory()) {
        node.close();
        return LittleFS.remove(path);
    }
    File child = node.openNextFile();
    bool ok = true;
    while (child) {
        char childPath[PATH_BUF];
        const char* name = baseName(child.name());
        child.close();
        if (snprintf(childPath, sizeof(childPath), "%s/%s", path, name) >=
            (int)sizeof(childPath)) {
            ok = false;
            break;
        }
        if (!removePath(childPath, (uint8_t)(depth + 1))) {
            ok = false;
            break;
        }
        child = node.openNextFile();
    }
    node.close();
    if (!ok) return false;
    return LittleFS.rmdir(path);
}

bool FileMgrMode::copyPath(const char* src, const char* dst, uint8_t depth) {
    if (depth > MAX_PATH_DEPTH) return false;
    File input = LittleFS.open(src, FILE_READ);
    if (!input) return false;
    if (input.isDirectory()) {
        input.close();
        if (!LittleFS.exists(dst) && !LittleFS.mkdir(dst)) return false;
        File dir = LittleFS.open(src);
        if (!dir || !dir.isDirectory()) {
            if (dir) dir.close();
            return false;
        }
        File child = dir.openNextFile();
        bool ok = true;
        while (child) {
            char childSrc[PATH_BUF], childDst[PATH_BUF];
            const char* name = baseName(child.name());
            child.close();
            if (snprintf(childSrc, sizeof(childSrc), "%s/%s", src, name) >=
                    (int)sizeof(childSrc) ||
                snprintf(childDst, sizeof(childDst), "%s/%s", dst, name) >=
                    (int)sizeof(childDst) ||
                !copyPath(childSrc, childDst, (uint8_t)(depth + 1))) {
                ok = false;
                break;
            }
            child = dir.openNextFile();
        }
        dir.close();
        return ok;
    }

    File output = LittleFS.open(dst, FILE_WRITE);
    if (!output) {
        input.close();
        return false;
    }
    uint8_t chunk[COPY_CHUNK];
    size_t left = input.size();
    bool ok = true;
    while (left) {
        size_t want = left > sizeof(chunk) ? sizeof(chunk) : left;
        size_t got = input.read(chunk, want);
        if (!got || output.write(chunk, got) != got) {
            ok = false;
            break;
        }
        left -= got;
        yield();
    }
    output.flush();
    output.close();
    input.close();
    if (!ok) LittleFS.remove(dst);
    return ok;
}

bool FileMgrMode::pasteClipboard() {
    if (clipboardAction == ClipboardAction::EMPTY || !clipboardPath[0]) {
        Display::showToast("CLIPBOARD EMPTY", 800);
        return false;
    }
    char target[PATH_BUF];
    if (snprintf(target, sizeof(target), "%s%s%s", curPath,
                 strcmp(curPath, "/") ? "/" : "", baseName(clipboardPath)) >=
        (int)sizeof(target)) {
        Display::showToast("PATH TOO LONG", 900);
        return false;
    }
    if (!strcmp(target, clipboardPath)) {
        Display::showToast("SAME LOCATION", 800);
        return false;
    }
    if (LittleFS.exists(target)) {
        phase = Phase::CONFIRM_OVERWRITE;
        strncpy(statusMsg, "REPLACE PASTE TARGET?", STATUS_BUF - 1);
        return false;
    }
    bool ok = false;
    if (clipboardAction == ClipboardAction::MOVE) {
        ok = LittleFS.rename(clipboardPath, target);
        if (ok) clipboardAction = ClipboardAction::EMPTY;
    } else {
        ok = copyPath(clipboardPath, target);
    }
    if (!ok) {
        Display::showToast("PASTE FAILED", 1000);
        return false;
    }
    refreshList();
    Display::showToast("PASTED", 700);
    return true;
}

void FileMgrMode::start() {
    phase = Phase::BROWSE;
    if (!allocateRuntimeBuffers()) {
        running = false;
        Display::showToast("LOW MEM - FILES", 1500);
        return;
    }
    running = true;
    strncpy(curPath, "/", PATH_BUF - 1);
    curPath[PATH_BUF - 1] = '\0';
    keyLatch = true;
    clipboardAction = ClipboardAction::EMPTY;
    clipboardPath[0] = '\0';
    refreshList();
    SFX::setMuted(false);
    SFX::stop();
    SFX::play(SFX::MODE_ENTER);
    Avatar::setState(AvatarState::HUNTING);
}

void FileMgrMode::stop() {
    running = false;
    freeRuntimeBuffers();
    clipboardAction = ClipboardAction::EMPTY;
    phase = Phase::BROWSE;
    SFX::setMuted(false);
    SFX::stop();
    Avatar::setState(AvatarState::NEUTRAL);
}

void FileMgrMode::back() {
    switch (phase) {
        case Phase::ACTIONS:
        case Phase::CONFIRM_DEL:
        case Phase::CONFIRM_OVERWRITE:
            phase = Phase::BROWSE;
            return;
        case Phase::IMAGE:
        case Phase::INFO:
            phase = Phase::BROWSE;
            refreshList();
            return;
        case Phase::VIEW:
            phase = Phase::BROWSE;
            freePreviewBuffer();
            refreshList();
            return;
        case Phase::BROWSE:
            if (strcmp(curPath, "/")) goUp();
            else stop();
            return;
    }
}

void FileMgrMode::buildActionList() {
    actionCount = 0;
    if (sel < entryCount) {
        actionItems[actionCount++] = Action::OPEN;
        actionItems[actionCount++] = Action::COPY;
        actionItems[actionCount++] = Action::MOVE;
        actionItems[actionCount++] = Action::DELETE;
    }
    if (clipboardAction != ClipboardAction::EMPTY)
        actionItems[actionCount++] = Action::PASTE;
    actionItems[actionCount++] = Action::REFRESH;
    if (strcmp(curPath, "/") != 0)
        actionItems[actionCount++] = Action::UP;
    actionItems[actionCount++] = Action::EXIT;
    actionSel = 0;
    actionScroll = 0;
    phase = Phase::ACTIONS;
}

void FileMgrMode::executeAction(Action action) {
    if (action == Action::OPEN) {
        phase = Phase::BROWSE;
        openSelected();
    } else if (action == Action::COPY || action == Action::MOVE) {
        if (sel >= entryCount) return;
        buildFullPath(clipboardPath, PATH_BUF, entries[sel].name);
        clipboardAction = action == Action::COPY
            ? ClipboardAction::COPY : ClipboardAction::MOVE;
        phase = Phase::BROWSE;
        Display::showToast(action == Action::COPY ? "COPIED TO CLIPBOARD" : "READY TO MOVE", 900);
    } else if (action == Action::PASTE) {
        phase = Phase::BROWSE;
        pasteClipboard();
    } else if (action == Action::DELETE) {
        if (sel < entryCount) phase = Phase::CONFIRM_DEL;
    } else if (action == Action::REFRESH) {
        phase = Phase::BROWSE;
        refreshList();
    } else if (action == Action::UP) {
        phase = Phase::BROWSE;
        goUp();
    } else if (action == Action::EXIT) {
        stop();
    }
}

void FileMgrMode::handleBrowseInput() {
    if (ButtonMapper::back()) {
        buildActionList();
        return;
    }
    if (ButtonMapper::prev()) {
        if (sel > 0) --sel;
        if (sel < scroll) scroll = sel;
        return;
    }
    if (ButtonMapper::next()) {
        if (entryCount && sel + 1 < entryCount) ++sel;
        if (sel >= scroll + VIS_ROWS) scroll = (uint16_t)(sel - VIS_ROWS + 1);
        return;
    }
    if (ButtonMapper::ok()) openSelected();
}

void FileMgrMode::handleActionsInput() {
    if (ButtonMapper::back()) {
        back();
        return;
    }
    if (ButtonMapper::prev()) {
        if (actionCount) {
            actionSel = actionSel == 0 ? (uint8_t)(actionCount - 1)
                                       : (uint8_t)(actionSel - 1);
            if (actionSel < actionScroll) actionScroll = actionSel;
            else if (actionSel >= actionScroll + ACTION_ROWS)
                actionScroll = (uint8_t)(actionSel - ACTION_ROWS + 1);
        }
    } else if (ButtonMapper::next()) {
        if (actionCount) {
            actionSel = (uint8_t)((actionSel + 1) % actionCount);
            if (actionSel >= actionScroll + ACTION_ROWS)
                actionScroll = (uint8_t)(actionSel - ACTION_ROWS + 1);
            else if (actionSel < actionScroll)
                actionScroll = actionSel;
        }
    } else if (ButtonMapper::ok() && actionCount) {
        executeAction(actionItems[actionSel]);
    }
}

void FileMgrMode::handleViewInput() {
    if (ButtonMapper::back() || ButtonMapper::ok()) {
        back();
        return;
    }
    if (ButtonMapper::prev()) {
        if (viewTopLine) --viewTopLine;
    } else if (ButtonMapper::next()) {
        uint16_t lines = 1;
        for (uint16_t i = 0; i < bufLen; ++i)
            if (buf[i] == '\n') ++lines;
        const uint16_t maxTop = lines > 6 ? (uint16_t)(lines - 6) : 0;
        if (viewTopLine < maxTop) ++viewTopLine;
    }
}

void FileMgrMode::update() {
    if (!running || App::windowHidden() || !ButtonMapper::keyNewPress(keyLatch)) return;

    if (phase == Phase::BROWSE) {
        handleBrowseInput();
    } else if (phase == Phase::ACTIONS) {
        handleActionsInput();
    } else if (phase == Phase::VIEW) {
        handleViewInput();
    } else if (phase == Phase::CONFIRM_DEL) {
        if (ButtonMapper::back() || ButtonMapper::prev()) {
            phase = Phase::BROWSE;
        } else if (ButtonMapper::ok()) {
            if (sel < entryCount) {
                char path[PATH_BUF];
                buildFullPath(path, sizeof(path), entries[sel].name);
                if (removePath(path)) {
                    Display::showToast("DELETED", 700);
                    refreshList();
                } else Display::showToast("DELETE FAILED", 1000);
            }
            phase = Phase::BROWSE;
        }
    } else if (phase == Phase::CONFIRM_OVERWRITE) {
        if (ButtonMapper::back() || ButtonMapper::prev()) {
            phase = Phase::BROWSE;
            return;
        }
        if (ButtonMapper::ok()) {
            char target[PATH_BUF];
            snprintf(target, sizeof(target), "%s%s%s", curPath,
                     strcmp(curPath, "/") ? "/" : "", baseName(clipboardPath));
            if (removePath(target)) {
                phase = Phase::BROWSE;
                pasteClipboard();
            } else {
                phase = Phase::BROWSE;
                Display::showToast("REPLACE FAILED", 1000);
            }
        }
    } else if (phase == Phase::IMAGE || phase == Phase::INFO) {
        if (ButtonMapper::back() || ButtonMapper::ok()) back();
    }
}

void FileMgrMode::getStatusLine(char* out, size_t n) {
    if (!out || !n) return;
    switch (phase) {
        case Phase::BROWSE:
            if (!curPath) snprintf(out, n, "LOW MEMORY");
            else if (listTruncated)
                snprintf(out, n, "PARTIAL LIST %u  LOW MEM", (unsigned)entryCount);
            else snprintf(out, n, "A/B SELECT  PWR OPEN  HOLD MENU");
            break;
        case Phase::ACTIONS:
            snprintf(out, n, "A/B SELECT  PWR RUN  HOLD BACK");
            break;
        case Phase::VIEW:
            snprintf(out, n, "A/B SCROLL  PWR BACK");
            break;
        case Phase::IMAGE:
            snprintf(out, n, "PWR BACK");
            break;
        case Phase::INFO:
            snprintf(out, n, "PWR BACK");
            break;
        case Phase::CONFIRM_DEL:
            snprintf(out, n, "PWR DELETE  B CANCEL");
            break;
        case Phase::CONFIRM_OVERWRITE:
            snprintf(out, n, "PWR REPLACE  B CANCEL");
            break;
    }
}

void FileMgrMode::drawBrowse(M5Canvas& canvas) {
    canvas.fillSprite(UiStyle::BG);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.setTextWrap(false);
    canvas.setTextColor(UiStyle::TITLE);
    uiDrawMarquee(canvas, curPath, 6, 2, 174);
    canvas.setTextColor(UiStyle::DIM);
    char counts[24];
    snprintf(counts, sizeof(counts), "%u/%u", entryCount ? (unsigned)sel + 1 : 0,
             (unsigned)entryCount);
    canvas.setTextDatum(top_right);
    canvas.drawString(counts, DISPLAY_W - 6, 2);
    canvas.setTextDatum(top_left);
    canvas.drawFastHLine(6, 13, DISPLAY_W - 12, UiStyle::CYAN);

    if (!curPath) {
        canvas.setTextColor(UiStyle::RED);
        canvas.drawString("LOW MEMORY", 8, 32);
        return;
    }
    if (!Storage::available()) {
        canvas.setTextColor(UiStyle::RED);
        canvas.drawString("NO SD CARD", 8, 28);
        canvas.setTextColor(UiStyle::DIM);
        canvas.drawString("INSERT CARD AND RESTART", 8, 42);
        return;
    }
    if (!entryCount) {
        canvas.setTextColor(UiStyle::DIM);
        canvas.drawString("(EMPTY FOLDER)", 8, 32);
    }
    for (uint16_t i = scroll; i < entryCount && i < scroll + VIS_ROWS; ++i) {
        int y = 17 + (i - scroll) * 14;
        bool selected = i == sel;
        if (selected) canvas.fillRoundRect(4, y - 1, DISPLAY_W - 8, 13, 2, UiStyle::PINK);
        canvas.setTextColor(selected ? UiStyle::BG :
                            entries[i].isDir ? UiStyle::GOLD : UiStyle::TEXT);
        char label[56];
        snprintf(label, sizeof(label), "%s %s",
                 entries[i].isDir ? "[D]" :
                 (imageType(entries[i].name) ? "[I]" :
                  (isTextFile(entries[i].name) ? "[T]" : "[F]")),
                 entries[i].name);
        uiDrawMarquee(canvas, label, 8, y + 2, 174);
        if (!entries[i].isDir) {
            char size[12];
            formatSize(size, sizeof(size), entries[i].size);
            canvas.setTextColor(selected ? UiStyle::BG : UiStyle::DIM);
            canvas.setTextDatum(top_right);
            canvas.drawString(size, DISPLAY_W - 8, y + 2);
            canvas.setTextDatum(top_left);
        }
    }

    if (entryCount && sel < entryCount) {
        int y = MAIN_H - 15;
        canvas.fillRect(0, y - 2, DISPLAY_W, 17, UiStyle::PANEL);
        canvas.setTextColor(UiStyle::CYAN);
        canvas.drawString(entries[sel].isDir ? "FOLDER" :
                          imageType(entries[sel].name) ? "IMAGE" :
                          isTextFile(entries[sel].name) ? "TEXT" : "FILE", 6, y);
        char size[16];
        formatSize(size, sizeof(size), entries[sel].size);
        canvas.setTextColor(UiStyle::DIM);
        canvas.setTextDatum(top_right);
        canvas.drawString(size, DISPLAY_W - 6, y);
        canvas.setTextDatum(top_left);
    }
}

void FileMgrMode::drawActions(M5Canvas& canvas) {
    canvas.fillSprite(UiStyle::BG);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.setTextWrap(false);
    canvas.setTextColor(UiStyle::TITLE);
    canvas.drawString("FILE ACTIONS", 6, 3);
    canvas.setTextColor(UiStyle::DIM);
    const char* selectedName = sel < entryCount ? entries[sel].name : curPath;
    uiDrawMarquee(canvas, selectedName, 6, 16, DISPLAY_W - 12);
    canvas.drawFastHLine(6, 28, DISPLAY_W - 12, UiStyle::PANEL);

    for (uint8_t row = 0; row < ACTION_ROWS && actionScroll + row < actionCount; ++row) {
        const uint8_t idx = (uint8_t)(actionScroll + row);
        const int y = 31 + row * 13;
        const bool selected = idx == actionSel;
        const char* label = "";
        switch (actionItems[idx]) {
            case Action::OPEN: label = "OPEN / VIEW"; break;
            case Action::COPY: label = "COPY HERE"; break;
            case Action::MOVE: label = "MOVE HERE"; break;
            case Action::PASTE: label = "PASTE HERE"; break;
            case Action::DELETE: label = "DELETE"; break;
            case Action::REFRESH: label = "REFRESH"; break;
            case Action::UP: label = "UP ONE FOLDER"; break;
            case Action::EXIT: label = "EXIT FILES"; break;
        }
        if (selected) canvas.fillRoundRect(4, y - 1, DISPLAY_W - 8, 12, 2, UiStyle::PINK);
        canvas.setTextColor(selected ? UiStyle::BG : UiStyle::TEXT);
        canvas.drawString(label, 9, y);
    }
}

void FileMgrMode::drawView(M5Canvas& canvas) {
    canvas.fillSprite(UiStyle::BG);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.setTextWrap(false);
    canvas.setTextColor(UiStyle::CYAN);
    uiDrawMarquee(canvas, openName, 6, 2, 204);
    canvas.drawFastHLine(6, 13, DISPLAY_W - 12, UiStyle::PANEL);

    uint16_t line = 0, i = 0, drawn = 0;
    const uint16_t top = viewTopLine;
    while (i <= bufLen && drawn < 6) {
        uint16_t start = i;
        while (i < bufLen && buf[i] != '\n') ++i;
        if (line >= top) {
            uint16_t len = (uint16_t)(i - start);
            if (len > 36) len = 36;
            char text[38];
            memcpy(text, buf + start, len);
            text[len] = '\0';
            int y = 17 + drawn * 13;
            canvas.setTextColor(UiStyle::TEXT);
            canvas.drawString(text, 7, y);
            ++drawn;
        }
        ++line;
        ++i;
    }
    if (bufLen == PREVIEW_CAP - 1) {
        canvas.setTextColor(UiStyle::GOLD);
        canvas.drawString("PREVIEW LIMITED TO 6 KB", 6, MAIN_H - 10);
    }
}

void FileMgrMode::drawImage(M5Canvas& canvas) {
    canvas.fillSprite(0x0000);
    char path[PATH_BUF];
    buildFullPath(path, sizeof(path), openName);
    bool ok = false;
    SDImageData image;
    if (hasExtension(openName, ".jpg") || hasExtension(openName, ".jpeg"))
        ok = canvas.drawJpgFile(&image, path, 0, 0, DISPLAY_W, MAIN_H,
                                0, 0, 0.0f, 0.0f);
    else if (hasExtension(openName, ".bmp"))
        ok = canvas.drawBmpFile(&image, path, 0, 0, DISPLAY_W, MAIN_H,
                                0, 0, 0.0f, 0.0f);
    else
        ok = canvas.drawPngFile(&image, path, 0, 0, DISPLAY_W, MAIN_H,
                                0, 0, 0.0f, 0.0f);
    if (!ok) {
        canvas.fillSprite(UiStyle::BG);
        canvas.setTextColor(UiStyle::RED);
        canvas.drawString("IMAGE DECODE FAILED", 8, 36);
        canvas.setTextColor(UiStyle::DIM);
        canvas.drawString(openName, 8, 50);
    } else {
        canvas.fillRect(0, 0, DISPLAY_W, 12, 0x0000);
        canvas.setTextColor(UiStyle::TEXT);
        uiDrawMarquee(canvas, openName, 5, 2, DISPLAY_W - 10);
    }
}

void FileMgrMode::draw(M5Canvas& canvas) {
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    if (phase == Phase::BROWSE) {
        drawBrowse(canvas);
        return;
    }
    if (phase == Phase::ACTIONS) {
        drawActions(canvas);
        return;
    }
    if (phase == Phase::VIEW) {
        drawView(canvas);
        return;
    }
    if (phase == Phase::IMAGE) {
        drawImage(canvas);
        return;
    }

    canvas.fillSprite(UiStyle::BG);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    if (phase == Phase::INFO && sel < entryCount) {
        const Entry& e = entries[sel];
        char size[16], path[PATH_BUF];
        formatSize(size, sizeof(size), e.size);
        buildFullPath(path, sizeof(path), e.name);
        canvas.setTextColor(UiStyle::TITLE);
        canvas.drawString("FILE DETAILS", 8, 8);
        canvas.setTextColor(UiStyle::TEXT);
        uiDrawMarquee(canvas, e.name, 8, 28, DISPLAY_W - 16);
        canvas.setTextColor(UiStyle::CYAN);
        canvas.drawString(size, 8, 44);
        canvas.setTextColor(UiStyle::DIM);
        uiDrawMarquee(canvas, path, 8, 60, DISPLAY_W - 16);
        canvas.setTextColor(UiStyle::GOLD);
        canvas.drawString("UNSUPPORTED PREVIEW FORMAT", 8, 78);
    } else if (phase == Phase::CONFIRM_OVERWRITE) {
        canvas.setTextColor(UiStyle::TITLE);
        canvas.drawString("REPLACE EXISTING?", 8, 15);
        canvas.fillRoundRect(6, 34, DISPLAY_W - 12, 20, 3, UiStyle::PANEL);
        canvas.setTextColor(UiStyle::TEXT);
        uiDrawMarquee(canvas, statusMsg, 10, 40, DISPLAY_W - 20);
    } else if (phase == Phase::CONFIRM_DEL && sel < entryCount) {
        canvas.setTextColor(UiStyle::RED);
        canvas.drawString(entries[sel].isDir ? "DELETE FOLDER AND CONTENTS?" : "DELETE FILE?", 8, 28);
        canvas.setTextColor(UiStyle::TEXT);
        uiDrawMarquee(canvas, entries[sel].name, 8, 48, DISPLAY_W - 16);
    }
}
