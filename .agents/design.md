---
version: alpha
name: CYD Wi-Fi Analyzer
colors:
  surface: "#181825"
  surface-dim: "#11111b"
  surface-bright: "#313244"
  surface-container-lowest: "#11111b"
  surface-container-low: "#181825"
  surface-container: "#1e1e2e"
  surface-container-high: "#313244"
  surface-container-highest: "#45475a"
  on-surface: "#cdd6f4"
  on-surface-variant: "#a6adc8"
  outline: "#6c7086"
  outline-variant: "#45475a"
  primary: "#89b4fa"
  on-primary: "#11111b"
  primary-container: "#313244"
  on-primary-container: "#89b4fa"
  secondary: "#b4befe"
  on-secondary: "#11111b"
  secondary-container: "#313244"
  on-secondary-container: "#b4befe"
  tertiary: "#cba6f7"
  on-tertiary: "#11111b"
  tertiary-container: "#313244"
  on-tertiary-container: "#cba6f7"
  error: "#f38ba8"
  on-error: "#11111b"
  error-container: "#313244"
  on-error-container: "#f38ba8"
  background: "#1e1e2e"
  on-background: "#cdd6f4"
typography:
  display-xl:
    fontFamily: Montserrat
    fontSize: 48px
    fontWeight: "700"
    lineHeight: 52px
  headline-lg:
    fontFamily: Montserrat
    fontSize: 28px
    fontWeight: "600"
    lineHeight: 34px
  title-md:
    fontFamily: Montserrat
    fontSize: 20px
    fontWeight: "600"
    lineHeight: 26px
  body-md:
    fontFamily: Montserrat
    fontSize: 14px
    fontWeight: "400"
    lineHeight: 20px
  label-sm:
    fontFamily: Montserrat
    fontSize: 10px
    fontWeight: "500"
    lineHeight: 14px
rounded:
  none: 0px
  sm: 4px
  md: 8px
  lg: 12px
  full: 9999px
spacing:
  none: 0px
  xs: 2px
  sm: 4px
  md: 8px
  lg: 12px
  xl: 16px
  gutter: 8px
components:
  spectrum-card:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.on-surface}"
    typography: "{typography.body-md}"
    rounded: "{rounded.md}"
    padding: "{spacing.sm}"
  header-bar:
    backgroundColor: "{colors.surface-container-lowest}"
    textColor: "{colors.on-surface}"
    typography: "{typography.body-md}"
    padding: "{spacing.sm}"
  button-primary:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.on-primary}"
    typography: "{typography.body-md}"
    rounded: "{rounded.sm}"
    padding: "{spacing.sm}"
  button-secondary:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.on-secondary}"
    typography: "{typography.body-md}"
    rounded: "{rounded.sm}"
    padding: "{spacing.sm}"
  dialog-modal:
    backgroundColor: "{colors.surface-container-low}"
    textColor: "{colors.on-surface}"
    typography: "{typography.title-md}"
    rounded: "{rounded.lg}"
    padding: "{spacing.md}"
  setting-switch:
    backgroundColor: "{colors.surface-container-high}"
    rounded: "{rounded.full}"
    padding: "{spacing.xs}"
  setting-slider:
    backgroundColor: "{colors.surface-container-highest}"
    rounded: "{rounded.sm}"
  badge-status:
    backgroundColor: "{colors.tertiary-container}"
    textColor: "{colors.on-tertiary-container}"
    typography: "{typography.label-sm}"
    rounded: "{rounded.full}"
    padding: "{spacing.xs}"
  alert-banner:
    backgroundColor: "{colors.error-container}"
    textColor: "{colors.on-error-container}"
    typography: "{typography.body-md}"
    rounded: "{rounded.sm}"
    padding: "{spacing.sm}"
---

## Brand & Style

The CYD Wi-Fi Analyzer design system delivers an embedded 2.4 GHz spectrum analyzer and network catalog interface tailored for ESP32-powered Cheap Yellow Display (CYD) hardware. The aesthetic blends the dark, pastel-accented Catppuccin palette with clean, legible typography optimized for small TFT displays (2.8" and 3.5" form factors).

The interface prioritizes instant readability from across a room while preserving tactile touch feedback. High contrast ensures clear visibility under varied lighting conditions, while the Catppuccin color harmony eliminates visual harshness during 24/7 desktop or lab operation.

## Colors

The color palette is built around Catppuccin Mocha as the default theme, with support for Macchiato, Frappe, and Latte flavors:

- **Primary (#89b4fa - Blue):** Drives interaction, highlights active toggles, and colors primary network metrics.
- **Secondary (#b4befe - Lavender):** Used for navigation tabs, subheadings, and secondary actions.
- **Tertiary (#cba6f7 - Mauve):** Accent color for modal title headers, badge indicators, and peak AP callouts.
- **Surface (#181825 - Mantle):** Elevated panel and spectrum plot background.
- **Background (#1e1e2e - Base):** The root canvas color, providing deep warmth without pure black clipping.
- **Error (#f38ba8 - Red):** Reserved for congested channels, reset buttons, and connection drops.
- **On-Surface (#cdd6f4 - Text):** Crisp light text delivering high WCAG contrast against all dark surfaces.
- **On-Surface-Variant (#a6adc8 - Subtext0):** Soft contrast for dBm units, BSSID strings, and timestamp metadata.
- **Outline (#6c7086 - Overlay0):** Subtle borders, spectrum frequency gridlines, and inactive switch tracks.

## Typography

The interface employs **Montserrat** rendered via LVGL's pre-compiled bitmap font engine (`LV_FONT_MONTSERRAT_*`). Montserrat's wide geometric proportions and clear apertures maintain exceptional legibility on low-DPI SPI LCD screens.

- **Headline-LG (28px, Semi-Bold):** Prominent dBm and rating callouts.
- **Title-MD (20px, Semi-Bold):** Section headers, tab view titles, and modal dialogue headers.
- **Body-MD (14px, Regular):** Standard body text, SSID labels, settings form labels, and button labels.
- **Label-SM (10px, Medium):** Spectrum channel markers (1-14), RSSI history chart axes, and small status indicators.

## Layout & Spacing

Layouts are designed for fixed-resolution displays with touch input:
- **CYD 2.8" Displays:** 320 x 240 pixels (Landscape).
- **CYD 3.5" Displays:** 480 x 320 pixels (Landscape) or 320 x 480 pixels (Portrait).

### Grid and Rhythm
- **Fixed Tab Header (34px):** Anchors navigation tabs (Spectrum, Networks, Channels, Settings).
- **Spacing Scale:** Built on a 4px modular unit (`xs: 2px`, `sm: 4px`, `md: 8px`, `lg: 12px`, `xl: 16px`) to maximize screen real estate while maintaining touch target ergonomics (minimum 32x32px touch targets).

## Components

The design system standardizes the following recurring components:

- **Spectrum View (`spectrum-card`):** Canvas drawing parabolic signal curves across channels 1–14 with dynamic Catppuccin color assignments and top-peak labels.
- **Networks List:** Scrollable list with channel badges, SSID, RSSI, and security icon buttons.
- **Detail Modal (`dialog-modal`):** Popup showing SSID, BSSID, security, frequency, live signal bar, and an 8-sample historical RSSI line chart.
- **Channels Table:** Formatted 4-column table listing Channel, AP count, Peak RSSI, and congestion rating.
- **Settings Controls (`setting-switch`, `setting-slider`):** Touch-friendly controls for configuring display brightness, auto-dimming, scan interval, passive scan, Catppuccin theme flavors, and screen orientation.
