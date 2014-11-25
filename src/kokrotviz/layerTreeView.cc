#include "layerTreeView.h"
using namespace kokrotviz;

LayerTreeView::ModelColumnRecord::ModelColumnRecord() 
{
    add(mNameColumn);
    add(mVisibleColumn);
    add(mOpacityColumn);
    add(mLineWidthColumn);
}


LayerTreeView::LayerTreeView() : mColumnRecord()
{
    mListStore = Gtk::ListStore::create(mColumnRecord);
}

void LayerTreeView::clear()
{
    mListStore->clear();
}

void LayerTreeView::populateFrom(LayerManager* Manager)
{
    Manager->visitLayers(Delegate<void(Layer&)>::fromCallable(
        [this] (Layer& l) {
            std::lock_guard<std::recursive_mutex> ll(l.Mutex);

            Gtk::TreeModel::iterator it = mListStore->append();
            Gtk::TreeModel::Row row = *it;

            row[mColumnRecord.mNameColumn] = "billy bob";
            row[mColumnRecord.mVisibleColumn] = l.getVisible();
            row[mColumnRecord.mOpacityColumn] = l.getOpacity();
            row[mColumnRecord.mLineWidthColumn] = 0.0;
        }));
}

void LayerTreeView::linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw)
{
    tvw->set_model(mListStore);
    tvw->append_column("Name", mColumnRecord.mNameColumn);
    tvw->append_column_editable("Visible", mColumnRecord.mVisibleColumn);
    tvw->append_column_editable("Opacity", mColumnRecord.mOpacityColumn);
    tvw->append_column_editable("Line width", mColumnRecord.mLineWidthColumn);
}
