#include "messageTreeView.h"
using namespace kokrotviz;

MessageTreeView::ModelColumnRecord::ModelColumnRecord() 
{
    add(mComponentColumn);
    add(mMessageTextColumn);
}


MessageTreeView::MessageTreeView() : mColumnRecord()
{
    mListStore = Gtk::ListStore::create(mColumnRecord);
}

void MessageTreeView::clear()
{
    mListStore->clear();
}

void MessageTreeView::addMessage(const std::string& msg)
{
    std::string component, message;
    if (msg.length() == 0)
        return;
    
    size_t closebracket = msg.find_first_of("]");
    if (msg[0] != ']' && closebracket == std::string::npos) {
        component = "";
        message = msg;
    } else {
        component = msg.substr(1, closebracket - 1);
        message = msg.substr(closebracket + 1);
    }

    Gtk::TreeModel::iterator it = mListStore->append();
    Gtk::TreeModel::Row row = *it;
    row[mColumnRecord.mComponentColumn] = Glib::ustring(component);
    row[mColumnRecord.mMessageTextColumn] = Glib::ustring(message);
}

void MessageTreeView::linkTreeView(Glib::RefPtr<Gtk::TreeView> tvw)
{
    tvw->set_model(mListStore);
    tvw->append_column("Component", mColumnRecord.mComponentColumn);
    tvw->append_column("Message", mColumnRecord.mMessageTextColumn);
}
