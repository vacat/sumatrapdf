/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#ifndef Controller_h
#define Controller_h

#include "DisplayState.h"

// call SaveThumbnail on success or delete ThumbnailCallback on failure
class ThumbnailCallback {
public:
    virtual ~ThumbnailCallback() { }
    virtual void SaveThumbnail(RenderedBitmap *bmp) = 0;
};

class DisplayModel;
class EbookController;
struct EbookFormattingData;

class ControllerCallback {
public:
    virtual ~ControllerCallback() { }
    // DisplayModel
    virtual void Repaint() = 0;
    virtual void PageNoChanged(int pageNo) = 0;
    virtual void UpdateScrollbars(SizeI canvas) = 0;
    virtual void RequestRendering(int pageNo) = 0;
    virtual void CleanUp(DisplayModel *dm) = 0;
    virtual void RenderThumbnail(DisplayModel *dm, SizeI size, ThumbnailCallback *tnCb) = 0;
    virtual void GotoLink(PageDestination *dest) = 0;
    // ChmEngine
    virtual void LaunchBrowser(const WCHAR *url) = 0;
    virtual void FocusFrame(bool always) = 0;
    virtual void SaveDownload(const WCHAR *url, const unsigned char *data, size_t len) = 0;
    // EbookController
    virtual void HandleLayoutedPages(EbookController *ctrl, EbookFormattingData *data) = 0;
    virtual void RequestDelayedLayout(int delay) = 0;
};

class FixedPageUIController;
class ChmUIController;
class EbookUIController;

class Controller {
protected:
    ControllerCallback *cb;

public:
    explicit Controller(ControllerCallback *cb) : cb(cb) { CrashIf(!cb); }
    virtual ~Controller() { }

    // meta data
    virtual const WCHAR *FilePath() const = 0;
    virtual const WCHAR *DefaultFileExt() const = 0;
    virtual int PageCount() const = 0;
    virtual WCHAR *GetProperty(DocumentProperty prop) = 0;

    // page navigation (stateful)
    virtual int CurrentPageNo() = 0;
    virtual void GoToPage(int pageNo, bool addNavPoint) = 0;
    virtual bool CanNavigate(int dir) = 0;
    virtual void Navigate(int dir) = 0;

    // view settings
    virtual void SetDisplayMode(DisplayMode mode, bool keepContinuous=true) = 0;
    virtual DisplayMode GetDisplayMode() const = 0;
    virtual void SetPresentationMode(bool enable) = 0;
    virtual void SetZoomVirtual(float zoom, PointI *fixPt=NULL) = 0;
    virtual float GetZoomVirtual() const = 0;
    virtual float GetNextZoomStep(float towards) const = 0;
    virtual void SetViewPortSize(SizeI size) = 0;

    // table of contents
    virtual bool HasTocTree() const = 0;
    virtual DocTocItem *GetTocTree() = 0;
    virtual void GotoLink(PageDestination *dest) = 0;
    virtual PageDestination *GetNamedDest(const WCHAR *name) = 0;

    // state export
    virtual void UpdateDisplayState(DisplayState *ds) = 0;
    // asynchronously calls ThumbnailCallback::SaveThumbnail (fails silently)
    virtual void CreateThumbnail(SizeI size, ThumbnailCallback *tnCb) = 0;

    // page labels (optional)
    virtual bool HasPageLabels() const { return false; }
    virtual WCHAR *GetPageLabel(int pageNo) const { return str::Format(L"%d", pageNo); }
    virtual int GetPageByLabel(const WCHAR *label) const { return _wtoi(label); }

    // common shortcuts
    virtual bool ValidPageNo(int pageNo) const {
        return 1 <= pageNo && pageNo <= PageCount();
    }
    virtual bool GoToNextPage() {
        if (CurrentPageNo() == PageCount())
            return false;
        GoToPage(CurrentPageNo() + 1, false);
        return true;
    }
    virtual bool GoToPrevPage(bool toBottom=false) {
        if (CurrentPageNo() == 1)
            return false;
        GoToPage(CurrentPageNo() - 1, false);
        return true;
    }
    virtual bool GoToFirstPage() {
        if (CurrentPageNo() == 1)
            return false;
        GoToPage(1, true);
        return true;
    }
    virtual bool GoToLastPage() {
        if (CurrentPageNo() == PageCount())
            return false;
        GoToPage(PageCount(), true);
        return true;
    }

    // for quick type determination and type-safe casting
    virtual FixedPageUIController *AsFixed() { return NULL; }
    virtual ChmUIController *AsChm() { return NULL; }
    virtual EbookUIController *AsEbook() { return NULL; }
};

class Synchronizer;
enum EngineType;

class FixedPageUIController : public Controller {
public:
    explicit FixedPageUIController(ControllerCallback *cb);
    virtual ~FixedPageUIController();

    virtual DisplayModel *model() = 0;
    virtual BaseEngine *engine() = 0;

    // controller-specific data (easier to save here than on WindowInfo)
    EngineType engineType;
    Vec<PageAnnotation> *userAnnots;
    bool userAnnotsModified;
    Synchronizer *pdfSync;

    static FixedPageUIController *Create(BaseEngine *engine, ControllerCallback *cb);
};

class ChmEngine;
class WindowInfo;

class ChmUIController : public Controller {
public:
    explicit ChmUIController(ControllerCallback *cb) : Controller(cb) { }

    virtual ChmEngine *engine() = 0;

    static ChmUIController *Create(ChmEngine *engine, ControllerCallback *cb);
};

class Doc;

class EbookUIController : public Controller {
public:
    explicit EbookUIController(ControllerCallback *cb) : Controller(cb) { }

    virtual Doc *doc() = 0;

    virtual LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam, bool& wasHandled) = 0;
    virtual void EnableMessageHandling(bool enable) = 0;
    virtual void UpdateDocumentColors() = 0;
    virtual void RequestRepaint() = 0;
    virtual void OnLayoutTimer() = 0;
    // EbookController's constructor calls UpdateWindow which
    // must not happen before EbookUIController::Create returns
    virtual EbookController *CreateController(DisplayMode displayMode) = 0;

    static EbookUIController *Create(HWND hwnd, ControllerCallback *cb);
};

#endif
