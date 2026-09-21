#include "ConnectionProfileStore.h"

#include <QByteArray>
#include <QSettings>

namespace {

const char *kArrayKey = "ConnectionProfiles";

QByteArray obfuscate(const QString &plain)
{
    QByteArray data = plain.toUtf8();
    for (int i = 0; i < data.size(); ++i) {
        data[i] = static_cast<char>(data[i] ^ 0x5A);
    }
    return data.toBase64();
}

QString deobfuscate(const QByteArray &stored)
{
    QByteArray data = QByteArray::fromBase64(stored);
    for (int i = 0; i < data.size(); ++i) {
        data[i] = static_cast<char>(data[i] ^ 0x5A);
    }
    return QString::fromUtf8(data);
}

} // namespace

QList<ConnectionProfile> ConnectionProfileStore::loadAll()
{
    QSettings settings;
    QList<ConnectionProfile> profiles;

    const int count = settings.beginReadArray(kArrayKey);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);

        ConnectionProfile profile;
        profile.name = settings.value("name").toString();
        profile.host = settings.value("host").toString();
        profile.port = static_cast<quint16>(settings.value("port", 443).toUInt());
        profile.hubAdminMode = settings.value("hubAdminMode", false).toBool();
        profile.hubName = settings.value("hubName").toString();
        profile.noSavePassword = settings.value("noSavePassword", false).toBool();
        if (!profile.noSavePassword) {
            profile.password = deobfuscate(settings.value("password").toByteArray());
        }

        profiles.append(profile);
    }
    settings.endArray();

    return profiles;
}

void ConnectionProfileStore::saveAll(const QList<ConnectionProfile> &profiles)
{
    QSettings settings;

    settings.beginWriteArray(kArrayKey);
    for (int i = 0; i < profiles.size(); ++i) {
        settings.setArrayIndex(i);

        const ConnectionProfile &profile = profiles.at(i);
        settings.setValue("name", profile.name);
        settings.setValue("host", profile.host);
        settings.setValue("port", profile.port);
        settings.setValue("hubAdminMode", profile.hubAdminMode);
        settings.setValue("hubName", profile.hubName);
        settings.setValue("noSavePassword", profile.noSavePassword);
        settings.setValue("password", profile.noSavePassword ? QByteArray() : obfuscate(profile.password));
    }
    settings.endArray();
}
