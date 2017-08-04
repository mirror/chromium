This directory contains shared components for Web UI. Currently these are
shared between Settings and stand alone dialogs, but may also be shared
between dialogs or with login UI.

These components are allowed to use I18nBehavior. The Web UI hosting these
lements is expected to provide loadTimeData with the necessary strings.
TODO(stevenjb/dschuyler): Add support for i18n{} substitution.

These components can not be used on ios.

