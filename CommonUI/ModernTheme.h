#ifndef MODERNTHEME_H
#define MODERNTHEME_H

#include <QString>
#include <QColor>

namespace ModernTheme {
    // Primary Colors (Bright Blue Theme from Reference - Figma Style)
    const QString PRIMARY_BLUE = "#3B82F6";         // Main blue for buttons and accents (Figma primary)
    const QString PRIMARY_BLUE_HOVER = "#60A5FA";   // Lighter blue for hover states
    const QString PRIMARY_BLUE_PRESSED = "#2563EB"; // Darker blue for pressed states
    const QString PRIMARY_FONT = "Segoe UI";        // Clean sans font to mirror the reference style
    
    // Message Bubble Colors (REVERSED from reference as per requirements)
    const QString OUTGOING_BUBBLE_BG = "#E5E7EB";  // Light medium gray for self messages (visible on light background)
    const QString OUTGOING_BUBBLE_TEXT = "#1F2937"; // Dark text on gray
    const QString INCOMING_BUBBLE_BG = "#3B82F6";   // Bright primary blue for others' messages
    const QString INCOMING_BUBBLE_TEXT = "#FFFFFF"; // White text on blue
    
    // Background Colors
    const QString CHAT_BACKGROUND = "#F4F5F7";     // Light gray chat area
    const QString SIDEBAR_BACKGROUND = "#FFFFFF";   // Clean white sidebars
    const QString HEADER_BACKGROUND = "#FFFFFF";    // White headers
    
    // Text Colors
    const QString PRIMARY_TEXT = "#2C2C2C";        // Main text color
    const QString SECONDARY_TEXT = "#666666";      // Secondary/muted text
    const QString PLACEHOLDER_TEXT = "#999999";    // Placeholder text
    
    // Border and Divider Colors
    const QString BORDER_COLOR = "#E0E0E0";        // Light borders
    const QString DIVIDER_COLOR = "#D0D0D0";       // Dividers
    
    // Component Specific
    const QString INPUT_BACKGROUND = "#FFFFFF";     // Input fields
    const QString BUTTON_TEXT = "#FFFFFF";          // Button text
    const QString HOVER_BACKGROUND = "#F5F7FF";     // Subtle blue tint for hovers
    const QString SUPPORT_GRADIENT_START = "#4AA3FF"; // Gradient for support banner
    const QString SUPPORT_GRADIENT_END = "#3B82F6";   // Gradient for support banner
    
    // Border Radius Values
    const int BUTTON_RADIUS = 20;                   // Highly rounded buttons
    const int INPUT_RADIUS = 25;                    // Very rounded input fields
    const int BUBBLE_RADIUS = 18;                   // Rounded message bubbles
    const int CARD_RADIUS = 12;                     // Cards and containers
}

#endif // MODERNTHEME_H
