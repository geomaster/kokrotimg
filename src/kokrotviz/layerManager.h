#ifndef __KOKROTVIZ_LAYERMANAGER_H__
#define __KOKROTVIZ_LAYERMANAGER_H__
#include "delegate.h"
#include <vector>
#include <mutex>
#include <gdkmm/pixbuf.h>
#include "eventSource.h"
#include <kokrotimg/kokrotimg.h>

namespace kokrotviz
{
    using ZIndex = int;

    struct Point
    {
        dimension X, Y;
    };
    
    struct Line
    {
        Point A, B;
    };

    struct Bitmap
    {
        int Width, Height;
        uint64_t GUID;
        byte* Data;
    };

    struct Polygon
    {
        dimension Size;
        Point* Data;
    };

    enum LayerElementType
    {
        LET_Bitmap,
        LET_Point,
        LET_Line,
        LET_Polygon
    };

    struct LayerElement 
    {
        LayerElementType Type;
        union {
            Point P;
            Bitmap B;
            Line L;
            Polygon Poly;
        } Data;
        std::string DebugString;
    };


    struct Color
    {
        int R, G, B;
    };

    class Layer
    {
    public:
        Layer();
        Layer(const Layer& other) : 
            mElements(other.mElements), mColor(other.mColor), mOpacity(other.mOpacity),
            mVisible(other.mVisible), mZIndex(other.mZIndex), mLineWidth(other.mLineWidth), Mutex() { ; }

        Layer& operator=(const Layer& other) {
            mElements = other.mElements;
            mColor = other.mColor;
            mOpacity = other.mOpacity;
            mVisible = other.mVisible;
            mZIndex = other.mZIndex;

        }

        Layer& operator=(Layer&& other) {
            mElements = std::move(other.mElements);
            mColor = other.mColor;
            mOpacity = other.mOpacity;
            mVisible = other.mVisible;
            mZIndex = other.mZIndex;
        }

        void setColor(Color C) { mColor = C; }
        void setOpacity(int Opacity) { mOpacity = Opacity; }
        void setVisible(bool Visible) { mVisible = Visible; }
        void setZIndex(ZIndex Z) { mZIndex = Z; }
        void setLineWidth(double LW) { mLineWidth = LW; }

        Color getColor() const { return mColor; }
        int getOpacity() const { return mOpacity; }
        bool getVisible() const { return mVisible; }
        ZIndex getZIndex() const { return mZIndex; }
        double getLineWidth() const { return mLineWidth; }

        const std::vector<LayerElement>& getElements() const { return mElements; }
        std::vector<LayerElement>& getElements() { return mElements; }

        mutable std::recursive_mutex Mutex;

    protected:
        Color mColor;
        int mOpacity;
        ZIndex mZIndex;
        bool mVisible;
        double mLineWidth;

        std::vector<LayerElement> mElements;
    }; 

    class LayerManager 
    {
    public:
        LayerManager();

        std::pair<dimension, dimension> getCanvasDimensions() const 
            { return std::make_pair(mWidth, mHeight); }
        void setCanvasDimensions(std::pair<dimension, dimension> NewDim) 
            { mWidth = NewDim.first; mHeight = NewDim.second; }

        void setLayerFilter(Delegate<bool(int)> Filter) { mFilter = Filter; mFilterActive = true; }
        void clearLayerFilter() { mFilterActive = false; }

        void visitLayers(Delegate<void(kok_debug_class, Layer&)> Visitor);
        std::pair<Layer*, LayerElement> getElementAt(Point P);

        EventSource<>& getLayersChangedEventSource() { return mLayerChangeES; }

        Layer& getLayer(int Idx);
        void clearLayers();
        void notifyZOrderChange() { mSorted = false; }
        void notifyLayersChanged() { mLayerChangeES.fire(); }

        ~LayerManager();

    protected:
        std::vector<std::pair<int, Layer> > mLayers;
        EventSource<> mLayerChangeES;
        Delegate<bool(int)> mFilter;

        int mWidth, mHeight;
        bool mSorted, mFilterActive;

        void onScanStarted();
        void onScanFinished();

        void sortIfNotSorted();
    };
}

#endif
