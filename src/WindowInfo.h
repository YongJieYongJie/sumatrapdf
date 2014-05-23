/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#ifndef WindowInfo_h
#define WindowInfo_h

#include "Controller.h"
// for DisplayModelCallback
#include "DisplayModel.h"

class Synchronizer;
class DoubleBuffer;
class SelectionOnPage;
class LinkHandler;
class Notifications;
class StressTest;
struct WatchedFile;
class SumatraUIAutomationProvider;
struct TabData;
class Controller;

/* Describes actions which can be performed by mouse */
enum MouseAction {
    MA_IDLE = 0,
    MA_DRAGGING,
    MA_DRAGGING_RIGHT,
    MA_SELECTING,
    MA_SCROLLING,
    MA_SELECTING_TEXT
};

enum PresentationMode {
    PM_DISABLED = 0,
    PM_ENABLED,
    PM_BLACK_SCREEN,
    PM_WHITE_SCREEN
};

// WM_GESTURE handling
struct TouchState {
    bool    panStarted;
    POINTS  panPos;
    int     panScrollOrigX;
    double  startArg;
};

/* Describes position, the target (URL or file path) and infotip of a "hyperlink" */
struct StaticLinkInfo {
    StaticLinkInfo() : target(NULL), infotip(NULL) { }
    StaticLinkInfo(RectI rect, const WCHAR *target, const WCHAR *infotip=NULL) :
        rect(rect), target(target), infotip(infotip) { }

    RectI rect;
    const WCHAR *target;
    const WCHAR *infotip;
};

/* Describes information related to one window with (optional) a document
   on the screen */
class WindowInfo : public DisplayModelCallback
{
public:
    explicit WindowInfo(HWND hwnd);
    ~WindowInfo();

    // TODO: error windows currently have
    //       !IsAboutWindow() && !IsDocLoaded()
    //       which doesn't allow distinction between PDF, XPS, etc. errors
    bool IsAboutWindow() const { return !loadedFilePath; }
    bool IsDocLoaded() const { return this->ctrl != NULL; }
    // TODO: consistent naming
    bool IsFixedDocLoaded() const { return ctrl && ctrl->AsFixed(); }
    bool IsChm() const { return ctrl && ctrl->AsChm(); }
    bool IsEbookLoaded() const { return ctrl && ctrl->AsEbook(); }

    FixedPageUIController *AsFixed() { return ctrl ? ctrl->AsFixed() : NULL; }
    ChmUIController *AsChm() { return ctrl ? ctrl->AsChm() : NULL; }
    EbookUIController *AsEbook() { return ctrl ? ctrl->AsEbook() : NULL; }

    EngineType GetEngineType() const { return ctrl && ctrl->AsFixed() ? ctrl->AsFixed()->engineType : Engine_None; }
    bool IsCbx() const { return GetEngineType() == Engine_ComicBook; } // needed for Manga mode
    bool IsNotPdf() const { return IsDocLoaded() && GetEngineType() != Engine_PDF; } // note: an error might be a PDF

    WCHAR *         loadedFilePath;
    Controller *    ctrl;

    HWND            hwndFrame;
    HWND            hwndCanvas;
    HWND            hwndToolbar;
    HWND            hwndReBar;
    HWND            hwndFindText;
    HWND            hwndFindBox;
    HWND            hwndFindBg;
    HWND            hwndPageText;
    HWND            hwndPageBox;
    HWND            hwndPageBg;
    HWND            hwndPageTotal;

    // state related to table of contents (PDF bookmarks etc.)
    HWND            hwndTocBox;
    HWND            hwndTocTree;
    bool            tocLoaded;
    bool            tocVisible;
    // set to temporarily disable UpdateTocSelection
    bool            tocKeepSelection;
    // an array of ids for ToC items that have been expanded/collapsed by user
    Vec<int>        tocState;
    DocTocItem *    tocRoot;

    // state related to favorites
    HWND            hwndFavBox;
    HWND            hwndFavTree;
    Vec<DisplayState *> expandedFavorites;

    // vertical splitter for resizing left side panel
    HWND            hwndSidebarSplitter;

    // horizontal splitter for resizing favorites and bookmars parts
    HWND            hwndFavSplitter;

    HWND            hwndTabBar;
    bool            tabsVisible;
    // keeps the sequence of tab selection. This is needed for restoration 
    // of the previous tab when the current one is closed.
    Vec<TabData *> *tabSelectionHistory;

    HWND            hwndInfotip;

    bool            infotipVisible;
    HMENU           menu;
    bool            isMenuHidden; // not persisted at shutdown

    int             dpi;
    float           uiDPIFactor;

    DoubleBuffer *  buffer;

    MouseAction     mouseAction;
    bool            dragStartPending;

    /* when dragging the document around, this is previous position of the
       cursor. A delta between previous and current is by how much we
       moved */
    PointI          dragPrevPos;
    /* when dragging, mouse x/y position when dragging was started */
    PointI          dragStart;

    /* when moving the document by smooth scrolling, this keeps track of
       the speed at which we should scroll, which depends on the distance
       of the mouse from the point where the user middle clicked. */
    int             xScrollSpeed, yScrollSpeed;

    bool            showSelection;

    /* selection rectangle in screen coordinates
     * while selecting, it represents area which is being selected */
    RectI           selectionRect;

    /* after selection is done, the selected area is converted
     * to user coordinates for each page which has not empty intersection with it */
    Vec<SelectionOnPage> *selectionOnPage;

    // a list of static links (mainly used for About and Frequently Read pages)
    Vec<StaticLinkInfo> staticLinks;

    // file change watcher
    WatchedFile *   watcher;

    bool            isFullScreen;
    PresentationMode presentation;
    // were we showing toc before entering full screen or presentation mode
    bool            tocBeforeFullScreen;
    int             windowStateBeforePresentation;

    long            nonFullScreenWindowStyle;
    RectI           nonFullScreenFrameRect;
    float           prevZoomVirtual;
    DisplayMode     prevDisplayMode;

    RectI           canvasRc; // size of the canvas (excluding any scroll bars)
    int             currPageNo; // cached value, needed to determine when to auto-update the ToC selection

    int             wheelAccumDelta;
    UINT_PTR        delayedRepaintTimer;

    Notifications * notifications; // only access from UI thread

    HANDLE          printThread;
    bool            printCanceled;

    HANDLE          findThread;
    bool            findCanceled;

    LinkHandler *   linkHandler;
    PageElement *   linkOnLastButtonDown;
    const WCHAR *   url;

    // synchronizer based on .pdfsync file
    Synchronizer *  pdfsync;

    /* when doing a forward search, the result location is highlighted with
     * rectangular marks in the document. These variables indicate the position of the markers
     * and whether they should be shown. */
    struct {
        bool show;          // are the markers visible?
        Vec<RectI> rects;   // location of the markers in user coordinates
        int page;
        int hideStep;       // value used to gradually hide the markers
    } fwdSearchMark;

    StressTest *    stressTest;

    TouchState      touchState;

    SumatraUIAutomationProvider * uia_provider;

    void  UpdateCanvasSize();
    SizeI GetViewPortSize();
    void  RedrawAll(bool update=false);
    void  RepaintAsync(UINT delay=0);

    void ChangePresentationMode(PresentationMode mode) {
        presentation = mode;
        RedrawAll();
    }

    void Focus() {
        if (IsIconic(hwndFrame))
            ShowWindow(hwndFrame, SW_RESTORE);
        SetFocus(hwndFrame);
    }

    void ToggleZoom();
    void MoveDocBy(int dx, int dy);

    void CreateInfotip(const WCHAR *text, RectI& rc, bool multiline=false);
    void DeleteInfotip();

    bool CreateUIAProvider();

    void FocusFrame(bool always);
    void SaveDownload(const WCHAR *url, const unsigned char *data, size_t len);

    // DisplayModelCallback implementation
    virtual void Repaint() { RepaintAsync(); };
    virtual void PageNoChanged(int pageNo);
    virtual void UpdateScrollbars(SizeI canvas);
    virtual void RequestRendering(int pageNo);
    virtual void CleanUp(DisplayModel *dm);
};

class LinkHandler {
    WindowInfo *owner;
    BaseEngine *engine() const;

    void ScrollTo(PageDestination *dest);
    void LaunchFile(const WCHAR *path, PageDestination *link);
    PageDestination *FindTocItem(DocTocItem *item, const WCHAR *name, bool partially=false);

public:
    explicit LinkHandler(WindowInfo& win) : owner(&win) { }

    void GotoLink(PageDestination *link);
    void GotoNamedDest(const WCHAR *name);
};

class LinkSaver : public LinkSaverUI {
    WindowInfo *owner;
    const WCHAR *fileName;

public:
    LinkSaver(WindowInfo& win, const WCHAR *fileName) : owner(&win), fileName(fileName) { }

    virtual bool SaveEmbedded(const unsigned char *data, size_t cbCount);
};

#endif
