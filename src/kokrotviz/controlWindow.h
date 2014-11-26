#ifndef __KOKROTVIZ_CONTROLWINDOW_H__
#define __KOKROTVIZ_CONTROLWINDOW_H__
#include "eventSource.h"
#include "layerTreeView.h"
#include "messageTreeView.h"
#include "metricsTreeView.h"
#include <glibmm/dispatcher.h>
#include "kokrotImgDriver.h"
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <string>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/radiobutton.h>

namespace kokrotviz {
    class VisualizationWindow;
    class ControlWindow {
        public:
            enum RealmType {
                Realm_Microcosm,
                Realm_Macrocosm
            };

            using ImageChosenEventSource = EventSource<const std::string&>;
            using ScanRequestedEventSource = EventSource<>;
            using RealmChangedEventSource = EventSource<RealmType>;

            ControlWindow(Gtk::ApplicationWindow *Window, Glib::RefPtr<Gtk::Builder> Builder, KokrotImgDriver *Driver, VisualizationWindow *VW);

            std::string getImageFilename() const;

            ImageChosenEventSource& getImageChosenEventSource()
            {
                return mImageChosenES;
            }

            ScanRequestedEventSource& getScanRequestedEventSource()
            {
                return mScanRequestedES;
            }

            RealmChangedEventSource& getRealmChangedEventSource()
            {
                return mRealmChangedES;
            }


        protected:
            Gtk::ApplicationWindow      *mWindow;
            Glib::RefPtr<Gtk::Builder>   mBuilder;
            Gtk::FileChooserButton      *mFC;
            Gtk::Button                 *mScanButton;
            Gtk::ScrolledWindow         *mMessagesScrolledWindow;
            Gtk::RadioButton            *mMicrocosmRadio, *mMacrocosmRadio;

            KokrotImgDriver             *mDriver;

            Glib::Dispatcher            mDispatcher;
            MessageTreeView             mTVW;
            MetricsTreeView             mMTVW;
            LayerTreeView               mLTVW;

            VisualizationWindow         *mVisWindow;

            RealmType                   mRealmType;

            bool mMetricsDirty, mScanActive;

            void onFileChosen();
            void onScanButtonClicked();
            void onNotification();

            ImageChosenEventSource mImageChosenES;
            ScanRequestedEventSource mScanRequestedES;
            RealmChangedEventSource mRealmChangedES;

            void onScanStarted();
            void onScanFinished(bool);

            void onMacrocosmSelected();
            void onMicrocosmSelected();

            std::mutex mMessageListMutex;
            int mScanFinishedFlag;
            std::vector<std::string> mMessageList;

            void changeRealm(RealmType);
    };

}



#endif
