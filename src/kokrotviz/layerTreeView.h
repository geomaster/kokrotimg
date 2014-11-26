#ifndef __KOKROTVIZ_LAYERTREEVIEW_H__
#define __KOKROTVIZ_LAYERTREEVIEW_H__
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include "layerManager.h"
#include <string>

namespace kokrotviz 
{
    class LayerTreeView
    {
    public:
        LayerTreeView();

        void linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw);
        void clear();
        void populateFrom(LayerManager* Manager);

        ~LayerTreeView();

    protected:
        class ModelColumnRecord : public Gtk::TreeModelColumnRecord
        {
        public:
            ModelColumnRecord();

            Gtk::TreeModelColumn<Glib::ustring> mNameColumn;
            Gtk::TreeModelColumn<int> mIndexColumn;
            Gtk::TreeModelColumn<bool> mVisibleColumn;
            Gtk::TreeModelColumn<double> mOpacityColumn,
                mLineWidthColumn;
        } mColumnRecord;

        void onRowChanged(const Gtk::TreePath path, const Gtk::TreeModel::iterator& iter);
        bool mPopulating;
        LayerManager* mManager;
        Glib::RefPtr<Gtk::TreeView> mActiveView;
        sigc::connection mConnection;

        Glib::RefPtr<Gtk::ListStore> mListStore;
    };
}


#endif 
