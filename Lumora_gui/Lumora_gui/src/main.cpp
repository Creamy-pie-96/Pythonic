/**
 * @file main.cpp
 * @brief Lumora GUI Demo Application
 * 
 * Demonstrates the Lumora GUI with mock data.
 * In production, replace MockDataProvider/MockCommandHandler
 * with your actual ML backend implementations.
 */

#include "core/LumoraAPI.hpp"
#include "panels/MainWindow.hpp"
#include "theme/NeonPalette.hpp"

#include <QApplication>
#include <QTimer>
#include <iostream>

using namespace lumora::gui;

int main(int argc, char *argv[])
{
    std::cout << R"(
    ╔═══════════════════════════════════════════════╗
    ║             LUMORA - Neural IDE               ║
    ║       Deep Learning Introspection Tool        ║
    ╚═══════════════════════════════════════════════╝
    )" << std::endl;
    
    // Create Qt application
    QApplication app(argc, argv);
    app.setApplicationName("Lumora");
    app.setApplicationVersion("0.1.0");
    app.setStyle("Fusion");
    
    // Apply dark palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, theme::colors::SPACE_GREY);
    darkPalette.setColor(QPalette::WindowText, theme::colors::FROST);
    darkPalette.setColor(QPalette::Base, theme::colors::CARBON);
    darkPalette.setColor(QPalette::AlternateBase, theme::colors::GRAPHITE);
    darkPalette.setColor(QPalette::ToolTipBase, theme::colors::GRAPHITE);
    darkPalette.setColor(QPalette::ToolTipText, theme::colors::FROST);
    darkPalette.setColor(QPalette::Text, theme::colors::FROST);
    darkPalette.setColor(QPalette::Button, theme::colors::CARBON);
    darkPalette.setColor(QPalette::ButtonText, theme::colors::FROST);
    darkPalette.setColor(QPalette::BrightText, theme::colors::PURE_WHITE);
    darkPalette.setColor(QPalette::Link, theme::colors::NEON_CYAN);
    darkPalette.setColor(QPalette::Highlight, theme::colors::NEON_CYAN);
    darkPalette.setColor(QPalette::HighlightedText, theme::colors::VOID_BLACK);
    app.setPalette(darkPalette);
    
    // Create mock provider and handler for demo
    MockDataProvider provider;
    MockCommandHandler handler;
    
    // Create main window
    MainWindow window(&provider, &handler);
    window.show();
    
    // Start simulated training after a short delay
    QTimer::singleShot(1000, [&provider]() {
        std::cout << "[Demo] Starting simulated training..." << std::endl;
        provider.startTraining();
    });
    
    // Add some demo ticker messages
    QTimer::singleShot(2000, [&window]() {
        window.pulseDashboard()->addTickerMessage("Training started", 
                                                   theme::colors::NEON_LIME);
    });
    
    QTimer::singleShot(5000, [&window]() {
        window.pulseDashboard()->addTickerMessage("Learning rate scheduled", 
                                                   theme::colors::NEON_CYAN);
    });
    
    // Run event loop
    return app.exec();
}
