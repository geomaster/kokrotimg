#include "kokrotImgDriver.h"
#include <gdkmm/pixbuf.h>
#include <glibmm/fileutils.h>
#include <kokrotimg/kokrotimg.h>
#include <dlfcn.h>
#include <iostream>
#include <iomanip>


using namespace kokrotviz;

static void driver_dbg_message(const char* p1, void* param) 
    { ((KokrotImgDriver*)param)->onDebugMessage(p1); }

static void driver_dbg_resize_canvas(dimension p1, dimension p2, void* param) 
    { ((KokrotImgDriver*)param)->onDebugResizeCanvas(p1, p2); }

static void driver_dbg_add_backdrop(const byte* p1, const char* p2, kok_debug_class p3, void* param) 
    { ((KokrotImgDriver*)param)->onDebugAddBackdrop(p1, p2, p3); }


static void driver_dbg_add_point(dimension p1, dimension p2, const char* p3, kok_debug_class p4, void* param) 
    { ((KokrotImgDriver*)param)->onDebugAddPoint(p1, p2, p3, p4); }

static void driver_dbg_add_line(dimension p1, dimension p2, dimension p3, dimension p4, const char* p5, kok_debug_class p6, void* param) 
    { ((KokrotImgDriver*)param)->onDebugAddLine(p1, p2, p3, p4, p5, p6); }

static void driver_dbg_add_polygon(const dimension* p1, const dimension* p2, int p3, int p4, const char* p5, kok_debug_class p6, void* param) 
    { ((KokrotImgDriver*)param)->onDebugAddPolygon(p1, p2, p3, p4, p5, p6); }

static void driver_dbg_record_metric(double p1, int p2, kok_metric_type p3, void* param) 
    { ((KokrotImgDriver*)param)->onDebugRecordMetric(p1, p2, p3); }

KokrotImgDriver::KokrotImgDriver(const std::string& LibraryPath) : mLibraryPath(""), mDLHandle(nullptr),
    mWidth(0), mHeight(0), mManager(nullptr)
{
    reloadLibrary(LibraryPath);
}

void KokrotImgDriver::reloadLibrary(const std::string& NewPath)
{
    std::string path = NewPath;
    if (path == "") { 
        path = mLibraryPath;
    }

    mDLHandle = dlopen(path.c_str(), RTLD_LAZY);
    if (!mDLHandle) {
        throw DynamicLibraryException(dlerror());
    }

    _sig_kokrot_api_version apiverfn = (_sig_kokrot_api_version) dlsym(mDLHandle, "kokrot_api_version");
    if (!apiverfn) {
        throw DynamicLibraryException("Symbol 'kokrot_api_version' not found");
    }

    std::string apiver = apiverfn();
    if (apiver != std::string(MY_KOKROT_API_VERSION)) {
        throw APIVersionMismatchException(std::string("Expected API version '") + MY_KOKROT_API_VERSION + 
                "' is different from library API version '" + apiver + "'");
    }

    const char* symnames[] = {
        "kokrot_initialize",
        "kokrot_set_debug_sink",
        "kokrot_cleanup",
        "kokrot_find_code"
    };

    for (int i = 0; i < KOKROTIMG_SYMBOL_COUNT; ++i) {
        mSymbols[i] = dlsym(mDLHandle, symnames[i]);
        if (!mSymbols[i]) {
            throw DynamicLibraryException(std::string("Symbol '") + symnames[i] + "' not found");
        }
    }

    mLibraryPath = path;
}

void KokrotImgDriver::loadImage(const std::string& ImagePath)
{
    Glib::RefPtr< Gdk::Pixbuf > pixbuf;
    try {
        pixbuf = Gdk::Pixbuf::create_from_file(ImagePath);
    } catch (Glib::FileError &e) {
        throw FileException(std::string("Could not load image file ") + ImagePath + ": " + e.what());
    } catch (Gdk::PixbufError &e) {
        throw InvalidImageException(std::string("Not a valid image file: ") + ImagePath + ": " + e.what());
    }

    if (pixbuf->get_colorspace() != Gdk::COLORSPACE_RGB || pixbuf->get_bits_per_sample() != 8) {
        throw InvalidImageException(std::string("Not a compatible image file: ") + ImagePath);
    }

    int w = pixbuf->get_width(), h = pixbuf->get_height(), d = pixbuf->get_n_channels(), area = w * h;
    guint8* pixels = pixbuf->get_pixels();

    mImageData.resize(area);

    mWidth = w;
    mHeight = h;
    for (int i = 0; i < area; ++i) {
        byte r = pixels[d * i],
             g = pixels[d * i + 1],
             b = pixels[d * i + 2];

        /* ITU-R rec. 709 coefficients */
        double luma = 0.2126 * r + 0.7152 * g + 0.0722 * b;

        int luma_int = (int) round(luma);
        if (luma_int < 0) {
            luma_int = 0;
        }
        if (luma_int > 255) {
            luma_int = 255;
        }

        mImageData[i] = luma_int;
    }
}

void KokrotImgDriver::scan()
{
    if (mWorker.joinable())
        mWorker.join();

    mWorker = std::thread([this] () {
        mScanStartedES.fire();
        mManager->clearLayers();

        kokrot_err_t ret;
        ret = ((_sig_kokrot_initialize)mSymbols[ kokrot_initialize ])();
        if (ret != kokrot_err_success) {
            mScanFinishedES.fire(false);
            return;
        }

        kok_debug_sink_t sink;
        sink.callback_param = this;
        sink.debug_message = &driver_dbg_message;
        sink.debug_resize_canvas = &driver_dbg_resize_canvas;
        sink.debug_add_backdrop = &driver_dbg_add_backdrop;
        sink.debug_add_point = &driver_dbg_add_point;
        sink.debug_add_line = &driver_dbg_add_line;
        sink.debug_add_polygon = &driver_dbg_add_polygon;
        sink.debug_record_metric = &driver_dbg_record_metric;

        mFullTime = 0.0;
        ((_sig_kokrot_set_debug_sink)mSymbols[ kokrot_set_debug_sink ])(&sink);

        ret = ((_sig_kokrot_find_code)mSymbols[ kokrot_find_code ])(&mImageData[0], mWidth, mHeight, NULL, NULL);
        ((_sig_kokrot_cleanup)mSymbols[ kokrot_cleanup ])();
        
        mScanFinishedES.fire(true);
    });
}

void KokrotImgDriver::onDebugMessage(const char* s)
{
    std::string str = s;
    mMessageReceivedES.fire(str);
}

void KokrotImgDriver::onDebugResizeCanvas(dimension w, dimension h)
{
    mWidth = w;
    mHeight = h;
    mManager->setCanvasDimensions(std::make_pair(w, h));
}

void KokrotImgDriver::onDebugAddBackdrop(const byte* data, const char* dbgstr, kok_debug_class clazz)
{
    static uint64_t BitmapGUID = 1;

    Layer& l = mManager->getLayer(clazz);

    /* Gdk/Cairo do not support anything other than 8-bit RGB in their pixmap data */
    byte * rgbdata = new byte[3 * mWidth * mHeight];
    for (int i = 0; i < mWidth * mHeight; ++i)
        rgbdata[3 * i] = rgbdata[3 * i + 1] = rgbdata[3 * i + 2] = data[i];

    Bitmap b = {
        .Width = mWidth,
        .Height = mHeight,
        .GUID = BitmapGUID++,
        .Data = rgbdata
    };

    LayerElement backdrop = {
        .Type = LET_Bitmap,
        .Data = { .B = b },
        .DebugString = (dbgstr? std::string(dbgstr) : std::string())
    };

    std::lock_guard<std::recursive_mutex> ll(l.Mutex);
    l.getElements().push_back(backdrop);
}

/* copy/paste lol */

void KokrotImgDriver::onDebugAddPoint(dimension x, dimension y, const char* dbgstr, kok_debug_class clazz)
{
    Layer& l = mManager->getLayer(clazz);
    Point p = { .X = x, .Y = y };

    LayerElement point = {
        .Type = LET_Point,
        .Data = { .P = p },
        .DebugString = (dbgstr? std::string(dbgstr) : std::string())
    };

    std::lock_guard<std::recursive_mutex> ll(l.Mutex);
    l.getElements().push_back(point);
}

void KokrotImgDriver::onDebugAddLine(dimension x0, dimension y0, dimension x1, dimension y1, const char* dbgstr, kok_debug_class clazz)
{
    Layer& l = mManager->getLayer(clazz);
    Line li = { .A = { .X = x0, .Y = y0 }, .B = { .X = x1, .Y = y1 } };

    LayerElement line = {
        .Type = LET_Line,
        .Data = { .L = li },
        .DebugString = (dbgstr? std::string(dbgstr) : std::string())
    };

    std::lock_guard<std::recursive_mutex> ll(l.Mutex);
    l.getElements().push_back(line);
}

void KokrotImgDriver::visitMetrics(Delegate<void(kok_metric_type, const Metric&)> Visitor)
{
    for (auto pair : mMetrics) 
        Visitor(pair.first, pair.second);
}

void KokrotImgDriver::onDebugRecordMetric(double time, int bytes, kok_metric_type type)
{
    Metric m = { time, bytes };
    mMetrics[type] = m;
}

void KokrotImgDriver::onDebugAddPolygon(const dimension* xs, const dimension* ys, int sz, int fill, 
        const char* dbgstr, kok_debug_class clazz)
{
    Layer& l = mManager->getLayer(clazz);
    
    Point* pts = new Point[sz];
    for (int i = 0; i < sz; ++i)
        pts[i] = { .X = xs[i], .Y = ys[i] };

    Polygon poly = {
        .Size = sz,
        .Data = pts
    };

    LayerElement polyelem = {
        .Type = LET_Polygon,
        .Data = { .Poly = poly },
        .DebugString = (dbgstr? std::string(dbgstr) : std::string())
    };

    std::lock_guard<std::recursive_mutex> ll(l.Mutex);
    l.getElements().push_back(polyelem);
}

KokrotImgDriver::~KokrotImgDriver()
{
    if (mWorker.joinable())
        mWorker.join();
}

