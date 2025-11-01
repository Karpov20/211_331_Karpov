#include "securitymanager.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#endif

namespace security {
namespace {

constexpr quint32 kIntegrityExpected = 0x17911B6A;
constexpr unsigned char kIntegrityBlock[] = "ShipmentLedgerIntegrityBlock_v1";

quint32 crc32(const quint8 *data, size_t length)
{
    quint32 crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const quint32 mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

#ifdef Q_OS_WIN
quint32 computeTextSectionChecksum()
{
    const HMODULE module = GetModuleHandleW(nullptr);
    if (!module) {
        return 0;
    }

    const auto *dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER *>(module);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    const auto *ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS *>(
        reinterpret_cast<const quint8 *>(module) + dosHeader->e_lfanew);
    if (!ntHeaders || ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(ntHeaders);
    for (unsigned i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        if (std::memcmp(section->Name, ".text", 5) == 0) {
            const quint8 *address = reinterpret_cast<const quint8 *>(
                reinterpret_cast<const quint8 *>(module) + section->VirtualAddress);
            const quint32 size = section->Misc.VirtualSize;
            return crc32(address, size);
        }
    }

    return 0;
}

bool isDebuggerPresentExtended()
{
    if (IsDebuggerPresent()) {
        return true;
    }

    BOOL remoteDebuggerPresent = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDebuggerPresent) && remoteDebuggerPresent) {
        return true;
    }

    using NtQueryInformationProcessFn = LONG(WINAPI *)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static const auto ntQueryInformationProcess =
        reinterpret_cast<NtQueryInformationProcessFn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),
                                                                     "NtQueryInformationProcess"));

    constexpr ULONG ProcessDebugPort = 7;
    constexpr ULONG ProcessDebugObjectHandle = 30;
    constexpr ULONG ProcessDebugFlags = 31;

    if (ntQueryInformationProcess) {
        ULONG_PTR debugPort = 0;
        if (ntQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
                                      &debugPort, sizeof(debugPort), nullptr) == 0 &&
            debugPort != 0) {
            return true;
        }

        HANDLE debugObject = nullptr;
        if (ntQueryInformationProcess(GetCurrentProcess(), ProcessDebugObjectHandle,
                                      &debugObject, sizeof(debugObject), nullptr) == 0 &&
            debugObject != nullptr) {
            return true;
        }

        ULONG debugFlags = 0;
        if (ntQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags,
                                      &debugFlags, sizeof(debugFlags), nullptr) == 0 &&
            (debugFlags & 1u) == 0) {
            return true;
        }
    }

    if (DebugActiveProcessStop(GetCurrentProcessId())) {
        return true;
    }

    return false;
}
#endif // Q_OS_WIN

class SecurityHelper : public QObject
{
public:
    explicit SecurityHelper(QObject *parent = nullptr)
        : QObject(parent)
    {
        if (!verifyStaticBlock()) {
            requestShutdown(QStringLiteral("Обнаружено изменение контрольной суммы статического блока."));
            return;
        }

#ifdef Q_OS_WIN
        m_textSectionChecksum = computeTextSectionChecksum();
        if (m_textSectionChecksum == 0) {
            requestShutdown(QStringLiteral("Не удалось инициализировать контрольную сумму приложения."));
            return;
        }

        startAntiDebugThread();
        scheduleIntegrityTimer();
#endif
    }

private:
    bool verifyStaticBlock() const
    {
        const auto checksum = crc32(reinterpret_cast<const quint8 *>(kIntegrityBlock),
                                    sizeof(kIntegrityBlock) - 1);
        return checksum == kIntegrityExpected;
    }

    void scheduleIntegrityTimer()
    {
#ifdef Q_OS_WIN
        auto *timer = new QTimer(this);
        timer->setInterval(3000);
        connect(timer, &QTimer::timeout, this, [this]() {
            const auto current = computeTextSectionChecksum();
            if (current == 0 || current != m_textSectionChecksum) {
                requestShutdown(QStringLiteral("Целостность кода нарушена."));
            }
        });
        timer->start();
#endif
    }

    void startAntiDebugThread()
    {
#ifdef Q_OS_WIN
        std::thread([this]() {
            while (!m_shutdownRequested.load(std::memory_order_acquire)) {
                if (isDebuggerPresentExtended()) {
                    requestShutdown(QStringLiteral("Обнаружена активная отладка процесса."));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
#endif
    }

    void requestShutdown(const QString &reason) const
    {
        const bool alreadyRequested = m_shutdownRequested.exchange(true, std::memory_order_acq_rel);
        if (alreadyRequested) {
            return;
        }

        QMetaObject::invokeMethod(qApp, [reason]() {
            QMessageBox::critical(nullptr, QStringLiteral("Защита приложения"), reason);
            QCoreApplication::exit(EXIT_FAILURE);
        }, Qt::QueuedConnection);
    }

    quint32 m_textSectionChecksum = 0;
    mutable std::atomic_bool m_shutdownRequested{false};
};

} // namespace

void installGuards()
{
    static SecurityHelper *helper = nullptr;
    if (!helper) {
        helper = new SecurityHelper(qApp);
    }
}

} // namespace security
