#include "layerManager.h"
#include <algorithm>
#include <cstdlib>
using namespace kokrotviz;


LayerManager::LayerManager() : mSorted(true), mWidth(0), mHeight(0)
{

}

Layer::Layer() : mVisible(false), mOpacity(255), mZIndex(0)
{
    /* FIXME: more pleasant random choosing! */
    mColor.R = rand() % 256;
    mColor.G = rand() % 256;
    mColor.B = rand() % 256;
}

void LayerManager::clearLayers()
{
    visitLayers(Delegate<void(Layer&)>::fromCallable(
        [] (Layer& l) {
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

void LayerManager::visitLayers(Delegate<void(Layer&)> Visitor)
{
    sortIfNotSorted();
    for (auto &a : mLayers) {
        Visitor(a.second);
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
