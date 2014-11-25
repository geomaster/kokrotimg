#ifndef __KOKROTVIZ_MESSAGETREEVIEW_H__
#define __KOKROTVIZ_MESSAGETREEVIEW_H__
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <string>

namespace kokrotviz 
{
    class MessageTreeView
    {
    public:
        MessageTreeView();

        void linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw);
        void clear();
        void addMessage(const std::string& msg);

    protected:
        class ModelColumnRecord : public Gtk::TreeModelColumnRecord
        {
        public:
            ModelColumnRecord();

            Gtk::TreeModelColumn<Glib::ustring> mComponentColumn, mMessageTextColumn;
        } mColumnRecord;

        Glib::RefPtr<Gtk::ListStore> mListStore;
    };
}


#endif 
