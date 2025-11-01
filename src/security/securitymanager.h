#pragma once

#include <QObject>

namespace security {

/// Installs runtime protections such as anti-debug and integrity monitoring.
void installGuards();

} // namespace security
