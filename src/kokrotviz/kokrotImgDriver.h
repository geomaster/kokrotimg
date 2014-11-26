#ifndef __KOKROTVIZ_KOKROTIMGDRIVER_H__
#define __KOKROTVIZ_KOKROTIMGDRIVER_H__
#include <string>
#include <vector>
#include "eventSource.h"
#include <mutex>
#include <map>
#include <thread>
#include <kokrotimg/kokrotimg.h>
#include "delegate.h"
#include "layerManager.h"
#include "exceptions.h"

#define MY_KOKROT_API_VERSION                       "1.0-arkona"
#define KOKROTIMG_SYMBOL_COUNT                      4

namespace kokrotviz
{
    class KokrotImgDriver {
    public:
        KokrotImgDriver(const std::string& LibraryPath);

        void loadImage(const std::string& ImagePath);
        void reloadLibrary(const std::string& NewPath = "");
        void scan();

        void setLayerManager(LayerManager* Mgr) { mManager = Mgr; }
        LayerManager* getLayerManager() { return mManager; }
        
        struct Metric {
            double TimeMs;
            int MemoryBytes;
        };

        void visitMetrics(Delegate<void(kok_metric_type, const Metric&)> Visitor);

        EventSource<>& getScanStartedEventSource() { return mScanStartedES; }
        EventSource<bool>& getScanFinishedEventSource() { return mScanFinishedES; }
        EventSource<const std::string&>& getMessageReceivedEventSource() { return mMessageReceivedES; }

        /* only Idx 0 (macro) and Idx 1 (micro) are meaningful */
        std::pair<dimension, dimension> getDimensionsFor(int Idx) { 
            if (Idx < 0 || Idx > 1) 
                return std::make_pair(-1, -1);

            return mDimensions[ Idx ];
        }

        void onDebugMessage(const char*);
        void onDebugResizeCanvas(dimension, dimension);
        void onDebugAddBackdrop(const byte*, const char*, kok_debug_class);
        void onDebugAddPoint(dimension, dimension, const char*, kok_debug_class);
        void onDebugAddLine(dimension, dimension, dimension, dimension, const char*, kok_debug_class);
        void onDebugAddPolygon(const dimension*, const dimension*, int, int, const char*, kok_debug_class);
        void onDebugRecordMetric(double, int, kok_metric_type);
     
        ~KokrotImgDriver();

    protected:
        void *mDLHandle;
        std::string mLibraryPath;
        std::vector<byte> mImageData;
        int mWidth, mHeight, mSeenFirstDimensions;
        LayerManager *mManager;
        
        std::map<kok_metric_type, Metric> mMetrics;
        std::pair<dimension, dimension> mDimensions[ 2 ];

        enum Symbols {
            kokrot_initialize,
            kokrot_set_debug_sink,
            kokrot_cleanup,
            kokrot_find_code
        };
        
        std::thread mWorker;

        void* mSymbols[KOKROTIMG_SYMBOL_COUNT];

        double mFullTime;
        EventSource<> mScanStartedES;
        EventSource<bool> mScanFinishedES;
        EventSource<const std::string&> mMessageReceivedES;

        typedef const char* (*_sig_kokrot_api_version) ();
        typedef kokrot_err_t (*_sig_kokrot_initialize) ();
        typedef void (*_sig_kokrot_set_debug_sink) (kok_debug_sink_t*);
        typedef kokrot_err_t (*_sig_kokrot_find_code) (byte*, dimension, dimension,
                byte*, dimension*);
        typedef void (*_sig_kokrot_cleanup) ();

       
    };

}

#endif
