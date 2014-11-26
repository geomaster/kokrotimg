#include "layerTreeView.h"
#include <iostream>
using namespace kokrotviz;

LayerTreeView::ModelColumnRecord::ModelColumnRecord() 
{
    add(mNameColumn);
    add(mIndexColumn);
    add(mVisibleColumn);
    add(mOpacityColumn);
    add(mLineWidthColumn);
}


LayerTreeView::LayerTreeView() : mColumnRecord(), mManager(nullptr), mActiveView(), mConnection(), mPopulating(false)
{
    mListStore = Gtk::ListStore::create(mColumnRecord);
    mConnection = mListStore->signal_row_changed().connect(sigc::mem_fun(*this, &LayerTreeView::onRowChanged));
}

void LayerTreeView::clear()
{
    mManager = nullptr;
    mListStore->clear();
}

void LayerTreeView::populateFrom(LayerManager* Manager)
{
    mManager = Manager;
    Manager->visitLayers(Delegate<void(kok_debug_class, Layer&)>::fromCallable(
        [this] (kok_debug_class clazz, Layer& l) {
            std::lock_guard<std::recursive_mutex> ll(l.Mutex);

            Gtk::TreeModel::iterator it = mListStore->append();
            Gtk::TreeModel::Row row = *it;

            std::string name = "Unknown";

#define DEFINE_MAPPING(cl, str) \
            case cl: { \
                name = (str); \
            } break

            switch (clazz) {
                DEFINE_MAPPING(KOKROTDBG_CLASS_MACRO_FINDER_CANDIDATE, "Finder candidates");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MACRO_FINDER_PATTERN_QUAD, "Finder pattern quads");
                DEFINE_MAPPING(KOKROTBDG_CLASS_MACRO_FINDER_PATTERN_POLYGON, "Finder pattern polygons");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MACRO_QR_CODE_QUAD, "QR code quads");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MACRO_SMALL_BFS_STARTPOINT, "Starting point for small BFS") ;
                DEFINE_MAPPING(KOKROTDBG_CLASS_MACRO_ORIGINAL_IMAGE, "Original image");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MACRO_BINARIZED_IMAGE, "Binarized image");

                DEFINE_MAPPING(KOKROTDBG_CLASS_MICRO_TIMING_PATTERN_LINES, "Timing pattern lines");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MICRO_FIRST_GRID, "Initial grid");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MICRO_QR_CODE_IMAGE, "Projected QR code");
                DEFINE_MAPPING(KOKROTDBG_CLASS_MICRO_BINARIZED_QR_CODE_IMAGE, "Binarized projected QR code");

                default: break;
            }


            struct myguard {
                LayerTreeView *v;
                myguard(LayerTreeView* p) : v(p) { p->mPopulating = true; }
                ~myguard() { std::cout << "yes, false." << std::endl; v->mPopulating = false; }
            } _g(this);
            
            row[mColumnRecord.mNameColumn] = name;
            row[mColumnRecord.mIndexColumn] = clazz;
            row[mColumnRecord.mVisibleColumn] = l.getVisible();
            row[mColumnRecord.mOpacityColumn] = l.getOpacity();
            row[mColumnRecord.mLineWidthColumn] = l.getLineWidth();
        }));
}

void LayerTreeView::onRowChanged(const Gtk::TreePath path, const Gtk::TreeModel::iterator& iter)
{
    if (mPopulating) 
        return;

    Gtk::TreeModel::Row row = *iter;
    Layer& l = mManager->getLayer(row[mColumnRecord.mIndexColumn]);
    l.setOpacity(row[mColumnRecord.mOpacityColumn]);
    l.setVisible(row[mColumnRecord.mVisibleColumn]);
    l.setLineWidth(row[mColumnRecord.mLineWidthColumn]);

    mManager->notifyLayersChanged();
}

void LayerTreeView::linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw)
{
    if (mActiveView)
        mActiveView->set_model(Glib::RefPtr<Gtk::TreeModel>());

    tvw->set_model(mListStore);
    tvw->append_column("ID", mColumnRecord.mIndexColumn);
    tvw->append_column("Name", mColumnRecord.mNameColumn);
    tvw->append_column_editable("Visible", mColumnRecord.mVisibleColumn);
    tvw->append_column_editable("Opacity", mColumnRecord.mOpacityColumn);
    tvw->append_column_editable("Line width", mColumnRecord.mLineWidthColumn);

}


LayerTreeView::~LayerTreeView()
{
    if (mActiveView) {
        mActiveView->set_model(Glib::RefPtr<Gtk::TreeModel>());
        mConnection.disconnect();
    }
}
