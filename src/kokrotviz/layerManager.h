#ifndef __KOKROTVIZ_LAYERMANAGER_H__
#define __KOKROTVIZ_LAYERMANAGER_H__
#include "delegate.h"
#include <vector>
#include <mutex>
#include <gdkmm/pixbuf.h>
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
            mVisible(other.mVisible), mZIndex(other.mZIndex), Mutex() { ; }

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

        Color getColor() const { return mColor; }
        int getOpacity() const { return mOpacity; }
        bool getVisible() const { return mVisible; }
        ZIndex getZIndex() const { return mZIndex; }

        const std::vector<LayerElement>& getElements() const { return mElements; }
        std::vector<LayerElement>& getElements() { return mElements; }

        mutable std::recursive_mutex Mutex;

    protected:
        Color mColor;
        int mOpacity;
        ZIndex mZIndex;
        bool mVisible;

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

        void visitLayers(Delegate<void(Layer&)> Visitor);
        std::pair<Layer*, LayerElement> getElementAt(Point P);

        Layer& getLayer(int Idx);
        void clearLayers();

        ~LayerManager();

    protected:
        std::vector<std::pair<int, Layer> > mLayers;
        int mWidth, mHeight;
        bool mSorted;

        void onScanStarted();
        void onScanFinished();

        void sortIfNotSorted();
    };
}

#endif
