#include "layerManager.h"
#include "utils.h"
#include <algorithm>
#include <cstdlib>
using namespace kokrotviz;


LayerManager::LayerManager() : mSorted(true), mWidth(0), mHeight(0), mFilterActive(false)
{

}

Layer::Layer() : mVisible(true), mOpacity(255), mZIndex(0), mLineWidth(5.0)
{
    static float h = 0.f;
    const float phireciproc = 0.618033989f;

    float s = 0.9f;
    float l = 0.8f;
    h = fmod(h + phireciproc * 359.f, 360.f);

    float r, g, b;
    Utils::hsvToRgb(h, s, l, &r, &g, &b);

    mColor.R = r * 255.0;
    mColor.G = g * 255.0;
    mColor.B = b * 255.0;
}


void LayerManager::clearLayers()
{
    visitLayers(Delegate<void(kok_debug_class, Layer&)>::fromCallable(
        [] (kok_debug_class, Layer& l) {
            std::lock_guard<std::recursive_mutex> ll(l.Mutex);
            for (auto elem : l.getElements()) {
                if (elem.Type == LET_Bitmap) {
                    delete[] elem.Data.B.Data;
                } else if (elem.Type == LET_Polygon) {
                    delete[] elem.Data.Poly.Data;
                }
            }
        }));

    mLayers.clear();
    mSorted = true;
}

Layer& LayerManager::getLayer(int Idx) 
{
    for (auto &a : mLayers) {
        if (a.first == Idx)
            return a.second;
    }

    mLayers.push_back(std::make_pair(Idx, Layer()));
    mSorted = false;
    return mLayers.back().second;
}

void LayerManager::visitLayers(Delegate<void(kok_debug_class, Layer&)> Visitor)
{
    sortIfNotSorted();
    for (auto &a : mLayers) {
        if (!mFilterActive || mFilter(a.first))
            Visitor(a.first, a.second);
    }
}

void LayerManager::sortIfNotSorted()
{
    if (mSorted)
        return;

    std::sort(mLayers.begin(), mLayers.end(), 
            [] (const std::pair<int, Layer>& a, const std::pair<int, Layer>& b) -> bool 
            {
                return ((Layer*)(&(a.second)))->getZIndex() < ((Layer*)(&(b.second)))->getZIndex();
            });
    mSorted = true;
}

LayerManager::~LayerManager()
{

}
