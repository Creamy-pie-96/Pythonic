/**
 * @file LumoraGUI.cpp
 * @brief LumoraGUI class implementation
 */

#include "core/LumoraAPI.hpp"
#include "panels/MainWindow.hpp"
#include "theme/NeonPalette.hpp"

#include <QApplication>
#include <QTimer>

namespace lumora::gui {

// ============================================================================
// LumoraGUI::Impl
// ============================================================================

struct LumoraGUI::Impl {
    IDataProvider* provider;
    ICommandHandler* handler;
    IEventCallback* eventCallback;
    
    MainWindow* mainWindow = nullptr;
    QApplication* app = nullptr;
    QTimer* updateTimer = nullptr;
    
    int refreshRate = 60;
    bool observerEnabled = true;
    bool darkMode = true;
    
    Impl(IDataProvider* p, ICommandHandler* h, IEventCallback* e)
        : provider(p), handler(h), eventCallback(e) {}
};

// ============================================================================
// LumoraGUI Implementation
// ============================================================================

LumoraGUI::LumoraGUI(IDataProvider* provider, 
                     ICommandHandler* handler,
                     IEventCallback* eventCallback)
    : m_impl(std::make_unique<Impl>(provider, handler, eventCallback))
{
}

LumoraGUI::~LumoraGUI() {
    if (m_impl->mainWindow) {
        delete m_impl->mainWindow;
    }
}

void LumoraGUI::show() {
    if (!m_impl->mainWindow) {
        m_impl->mainWindow = new MainWindow(m_impl->provider, m_impl->handler);
    }
    m_impl->mainWindow->show();
}

void LumoraGUI::hide() {
    if (m_impl->mainWindow) {
        m_impl->mainWindow->hide();
    }
}

bool LumoraGUI::isVisible() const {
    return m_impl->mainWindow && m_impl->mainWindow->isVisible();
}

void LumoraGUI::update() {
    if (m_impl->app) {
        m_impl->app->processEvents();
    }
}

int LumoraGUI::exec(int argc, char* argv[]) {
    // Create QApplication if not exists
    if (!QApplication::instance()) {
        m_impl->app = new QApplication(argc, argv);
    }
    
    // Set application-wide style
    QApplication::setStyle("Fusion");
    
    // Create and show main window
    show();
    
    // Run event loop
    return QApplication::exec();
}

MainWindow* LumoraGUI::getMainWindow() {
    return m_impl->mainWindow;
}

void LumoraGUI::setRefreshRate(int hz) {
    m_impl->refreshRate = std::clamp(hz, 1, 120);
    if (m_impl->updateTimer) {
        m_impl->updateTimer->setInterval(1000 / m_impl->refreshRate);
    }
}

void LumoraGUI::setObserverEnabled(bool enabled) {
    m_impl->observerEnabled = enabled;
    if (m_impl->mainWindow) {
        m_impl->mainWindow->observer()->setVisible(enabled);
    }
}

void LumoraGUI::setDarkMode(bool dark) {
    m_impl->darkMode = dark;
    // Theme switching would be implemented here
    // For now, only dark mode is supported
}

} // namespace lumora::gui
