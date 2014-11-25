#include <gtkmm.h>
#include <stdlib.h>
#include <kokrotimg/kokrotimg.h>
#include <iostream>
#include "controlWindow.h"
#include "visualizationWindow.h"
#include "kokrotImgDriver.h"
#include "layerManager.h"

Gtk::ApplicationWindow* pWindow = 0;
Gtk::Window *pVisWindow = 0;

void imagechosen(const std::string &filename)
{

    std::cout << "chosen image " << filename << std::endl;
}

void scanreq()
{
    std::cout << "scan requested" << std::endl;

}

int main (int argc, char **argv)
{
    Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.kokrotimg.kokrotviz");

    //Load the GtkBuilder file and instantiate its widgets:
    Glib::RefPtr<Gtk::Builder> refBuilder = Gtk::Builder::create();

    srand(time(NULL));
    try {
        if (!refBuilder->add_from_resource("/org/kokrotimg/kokrotviz/kokrotviz.glade"))
            return( 1 );
    } catch(const Glib::MarkupError& ex) {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
        return( 1 );
    } catch(const Gtk::BuilderError& ex) {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
        return( 1 );
    } catch (const Gio::ResourceError& ex) {
        std::cerr << "ResourceError: " << ex.what() << std::endl;
        return( 1 );
    }

    refBuilder->get_widget("controlWindow", pWindow);
    refBuilder->get_widget("visWindow", pVisWindow);
    if(pWindow)
    {
        kokrotviz::LayerManager layerMgr;

        kokrotviz::KokrotImgDriver driver("lib/libkokrotimg.so");
        driver.setLayerManager(&layerMgr);

        kokrotviz::VisualizationWindow vw(pVisWindow, refBuilder, &layerMgr);
        kokrotviz::ControlWindow cw(pWindow, refBuilder, &driver, &vw);

        pVisWindow->show();
        app->run(*pWindow);
    }

    delete pWindow;
    delete pVisWindow;

    return 0;
}
