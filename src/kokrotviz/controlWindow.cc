#include "controlWindow.h"
#include "exceptions.h"
#include "utils.h"
#include <gtkmm/treeview.h>
#include "visualizationWindow.h"
#include <iostream>

using namespace kokrotviz;

ControlWindow::ControlWindow(Gtk::ApplicationWindow *Window, Glib::RefPtr<Gtk::Builder> Builder, 
        KokrotImgDriver *Driver, VisualizationWindow* VW) :
    mWindow(Window), mBuilder(Builder), mDriver(Driver), mDispatcher(), mMetricsDirty(false), mScanActive(false),
    mVisWindow(VW), mRealmType(Realm_Macrocosm)
{
    mFC = Utils::tryFindWidget<Gtk::FileChooserButton>("imageFileChooser", Builder);
    mFC->signal_file_set().connect(sigc::mem_fun(*this, &ControlWindow::onFileChosen));

    mScanButton = Utils::tryFindWidget<Gtk::Button>("imageScanButton", Builder);
    mScanButton->signal_clicked().connect(sigc::mem_fun(*this, &ControlWindow::onScanButtonClicked));
    
    mMicrocosmRadio = Utils::tryFindWidget<Gtk::RadioButton>("realmMicrocosm", Builder);
    mMicrocosmRadio->signal_clicked().connect(sigc::mem_fun(*this, &ControlWindow::onMicrocosmSelected));

    mMacrocosmRadio = Utils::tryFindWidget<Gtk::RadioButton>("realmMacrocosm", Builder);
    mMacrocosmRadio->signal_clicked().connect(sigc::mem_fun(*this, &ControlWindow::onMacrocosmSelected));

    Glib::RefPtr<Gtk::TreeView> tvw = Glib::RefPtr<Gtk::TreeView>(Utils::tryFindWidget<Gtk::TreeView>("messagesTree", Builder));

    Glib::RefPtr<Gtk::TreeView> mtvw = Glib::RefPtr<Gtk::TreeView>(Utils::tryFindWidget<Gtk::TreeView>("metricsTree", Builder));

    Glib::RefPtr<Gtk::TreeView> ltvw = Glib::RefPtr<Gtk::TreeView>(Utils::tryFindWidget<Gtk::TreeView>("layersTree", Builder));

    mMessagesScrolledWindow = Utils::tryFindWidget<Gtk::ScrolledWindow>("messagesScrolledWindow", Builder);

    mImageChosenES.addHandler(Delegate<void(const std::string&)>::fromCallable(
        [this] (const std::string& fname) {
            mScanButton->set_sensitive(true);

            try {
                mDriver->loadImage(fname);
            } catch (kokrotviz::Exception& e) {
                std::cout << "Kurac!\n" << e.what() << std::endl;
                mScanButton->set_sensitive(false);
            }
        }));

    mScanRequestedES.addHandler(Delegate<void()>::fromCallable(
        [this] () {
            try {
                mScanActive = true;

                mFC->set_sensitive(false);
                mScanButton->set_sensitive(false);
                mMicrocosmRadio->set_sensitive(false);
                mMacrocosmRadio->set_sensitive(false);

                mDriver->scan();
            } catch (kokrotviz::Exception& e) {
                mFC->set_sensitive(true);
                mScanButton->set_sensitive(true);
                mMicrocosmRadio->set_sensitive(true);
                mMacrocosmRadio->set_sensitive(true);

                mScanActive = false;

                std::cout << "Kurchina!\n" << e.what() << std::endl;
            }
        }));

    mDispatcher.connect(sigc::mem_fun(*this, &ControlWindow::onNotification));
    mDriver->getMessageReceivedEventSource().addHandler(Delegate<void(const std::string&)>::fromCallable(
        [this] (const std::string& msg) {
            {
                std::cout << msg << std::endl;

                std::lock_guard<std::mutex> ll(mMessageListMutex);
                mMessageList.push_back(msg);
            }
            mDispatcher.emit();
        }));

    mDriver->getScanStartedEventSource().addHandler(Delegate<void()>::fromCallable(
        [this] () {
            mTVW.clear();
            
        }));

    mDriver->getScanFinishedEventSource().addHandler(Delegate<void(bool)>::fromCallable(
        [this] (bool successful) {

            mScanActive = false;

            /* kludge alert */
            mDispatcher.emit();

            {
                std::lock_guard<std::mutex> ll(mMessageListMutex);
                mScanFinishedFlag = true;
            }
            mDispatcher.emit();
        }));

    mRealmChangedES.addHandler(Delegate<void(RealmType)>::fromCallable(
        [this] (RealmType t) {
            changeRealm(t);
        }));

    mDriver->getLayerManager()->getLayersChangedEventSource().addHandler(Delegate<void()>::fromCallable(
        [this] () {
            mVisWindow->redraw();
        }));

    mTVW.linkTreeView(tvw);
    mMTVW.linkTreeView(mtvw);
    mLTVW.linkTreeView(ltvw);
}
            

void ControlWindow::onFileChosen()
{
    mImageChosenES.fire(mFC->get_filename());
}

void ControlWindow::onScanButtonClicked()
{
    mScanRequestedES.fire();
}

void ControlWindow::onNotification()
{
    std::lock_guard<std::mutex> ll(mMessageListMutex);
    for (auto& m : mMessageList)
        mTVW.addMessage(m);

    mMessageList.clear();
    Glib::RefPtr<Gtk::Adjustment> adj = mMessagesScrolledWindow->get_vadjustment();
    adj->set_value(adj->get_upper()); 

    if (!mScanActive  && mScanFinishedFlag) {
        mScanFinishedFlag = false;

        mFC->set_sensitive(true);
        mScanButton->set_sensitive(true);
        mMicrocosmRadio->set_sensitive(true);
        mMacrocosmRadio->set_sensitive(true);
        
        double timeSum1 = 0.0, timeSum2 = 0.0;
        int memorySum1 = 0, memorySum2 = 0;
        mDriver->visitMetrics(Delegate<void(kok_metric_type, const KokrotImgDriver::Metric&)>::fromCallable(
            [&timeSum1, &memorySum1, &timeSum2, &memorySum2] (kok_metric_type type, const KokrotImgDriver::Metric& m) {
                if (type >= KOKROTDBG_METRIC_MICRO_BASE) {
                    timeSum2 += m.TimeMs;
                    memorySum2 = std::max(m.MemoryBytes, memorySum2);
                } else {
                    timeSum1 += m.TimeMs;
                    memorySum1 = std::max(m.MemoryBytes, memorySum1);
                }
            }));

        mMTVW.initializeView(timeSum1, memorySum1, timeSum2, memorySum2);

        mDriver->visitMetrics(Delegate<void(kok_metric_type, const KokrotImgDriver::Metric&)>::fromCallable(
            [this] (kok_metric_type type, const KokrotImgDriver::Metric& m) {
                mMTVW.addMetric(type, m.TimeMs, m.MemoryBytes);
            }));

        changeRealm(mRealmType);
    }
}

void ControlWindow::changeRealm(RealmType t)
{
    std::cout << "changing realm to t " << t << std::endl;
    mRealmType = t;

    mDriver->getLayerManager()->setCanvasDimensions(mDriver->getDimensionsFor(t == Realm_Macrocosm ? 0 : 1));
    mDriver->getLayerManager()->setLayerFilter(
            Delegate<bool(int)>::fromCallable(
                    [t] (int l) -> bool {
                        return (t == Realm_Macrocosm ? l < KOKROTDBG_CLASS_MICRO_BASE : l >= KOKROTDBG_CLASS_MICRO_BASE);
                    }));

    mLTVW.clear();
    mLTVW.populateFrom(mDriver->getLayerManager());

    mVisWindow->notifyLayersChange();
    mVisWindow->redraw();
}

void ControlWindow::onMacrocosmSelected()
{
    if (mMacrocosmRadio->get_active())
        mRealmChangedES.fire(Realm_Macrocosm);
}

void ControlWindow::onMicrocosmSelected()
{
    if (mMicrocosmRadio->get_active())
        mRealmChangedES.fire(Realm_Microcosm);
}

