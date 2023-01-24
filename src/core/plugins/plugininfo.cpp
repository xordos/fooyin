/*
 * Fooyin
 * Copyright 2022-2023, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "plugininfo.h"

#include "plugin.h"

#include <QPluginLoader>
#include <utility>

namespace Plugins {
struct PluginInfo::Private
{
    QString name;
    QString filename;
    QJsonObject metadata;
    QString version;
    QString vendor;
    QString copyright;
    QString license;
    QString category;
    QString description;
    QString url;
    bool isRequired{false};
    bool isLoaded{false};
    bool isDisabled{false};
    Status status{Invalid};
    QString error;

    Plugin* plugin{nullptr};
    QPluginLoader loader;

    Private(QString name, QString filename, const QJsonObject& allMetaData)
        : name{std::move(name)}
        , filename{std::move(filename)}
        , metadata{allMetaData.value("MetaData").toObject()}
        , version{metadata.value("Version").toString()}
        , vendor{metadata.value("Vendor").toString()}
        , copyright{metadata.value("Copyright").toString()}
        , license{metadata.value("License").toString()}
        , category{metadata.value("Category").toString()}
        , description{metadata.value("Description").toString()}
        , url{metadata.value("Url").toString()}
    { }
};

PluginInfo::PluginInfo(const QString& name, const QString& filename, const QJsonObject& metadata)
    : p{std::make_unique<Private>(name, filename, metadata)}
{
    p->loader.setFileName(filename);
}

PluginInfo::~PluginInfo() = default;

void PluginInfo::load()
{
    if(p->loader.fileName().isEmpty()) {
        return;
    }

    if(!p->loader.load()) {
        p->error = QString("Plugin %1 couldn't be loaded (%2)").arg(p->name, p->error);
        return;
    }

    p->plugin = qobject_cast<Plugin*>(p->loader.instance());

    if(!p->plugin) {
        p->error = QString("Plugin %1 couldn't be loaded").arg(p->name);
        return;
    }

    p->status   = Loaded;
    p->isLoaded = true;
}

void PluginInfo::unload()
{
    if(!p->plugin) {
        return;
    }
    p->plugin->shutdown();
    const bool deleted = p->loader.unload();
    if(!deleted) {
        delete p->plugin;
    }
}

void PluginInfo::initialise()
{
    p->plugin->initialise();
    p->status = Initialised;
}

Plugin* PluginInfo::plugin() const
{
    return p->plugin;
}

QString PluginInfo::name() const
{
    return p->name;
}

QString PluginInfo::filename() const
{
    return p->filename;
}

QJsonObject PluginInfo::metadata() const
{
    return p->metadata;
}

bool PluginInfo::isLoaded() const
{
    return p->isLoaded;
}

bool PluginInfo::isDisabled() const
{
    return p->isDisabled;
}

PluginInfo::Status PluginInfo::status() const
{
    return p->status;
}

QString PluginInfo::error() const
{
    return p->error;
}

bool PluginInfo::hasError() const
{
    return !p->error.isEmpty();
}

void PluginInfo::setError(const QString& error)
{
    p->error = error;
}

QString PluginInfo::version() const
{
    return p->version;
}

QString PluginInfo::identifier() const
{
    return (p->vendor + "." + p->name).toLower();
}

QString PluginInfo::category() const
{
    return p->category;
}

QString PluginInfo::copyright() const
{
    return p->copyright;
}

QString PluginInfo::description() const
{
    return p->description;
}

QString PluginInfo::url() const
{
    return p->url;
}
} // namespace Plugins
