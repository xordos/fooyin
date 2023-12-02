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

#include "filtersplugin.h"

#include "filtermanager.h"
#include "filterwidget.h"
#include "settings/filtersettings.h"
#include "settings/filtersfieldspage.h"
#include "settings/filtersgeneralpage.h"

#include <gui/layoutprovider.h>
#include <gui/plugins/guiplugincontext.h>
#include <gui/searchcontroller.h>
#include <gui/widgetfactory.h>
#include <utils/actions/actioncontainer.h>

namespace Fooyin::Filters {
struct FiltersPlugin::Private
{
    ActionManager* actionManager;
    SettingsManager* settings;
    MusicLibrary* library;
    PlayerManager* playerManager;
    LayoutProvider* layoutProvider;
    SearchController* searchController;
    WidgetFactory* factory;
    TrackSelectionController* trackSelection;

    FilterManager* filterManager;
    std::unique_ptr<FiltersSettings> filterSettings;

    std::unique_ptr<FiltersGeneralPage> generalPage;
    std::unique_ptr<FiltersFieldsPage> fieldsPage;

    void registerLayouts() const
    {
        layoutProvider->registerLayout(R"({"Obsidian":[{"SplitterVertical":{"Children":["StatusBar","SearchBar",{
            "SplitterHorizontal":{"Children":[{"FilterWidget":{"Sort":"AscendingOrder","Type":"Album Artist"}},
            {"FilterWidget":{"Sort":"AscendingOrder","Type":"Album"}},"Playlist",{
            "SplitterVertical":{"Children":["ArtworkPanel","InfoPanel"],
            "State":"AAAA/wAAAAEAAAACAAABcgAAAg4A/////wEAAAACAA=="}}],
            "State":"AAAA/wAAAAEAAAAEAAAA+wAAAVoAAAN6AAABcwD/////AQAAAAEA"}},"ControlBar"],
            "State":"AAAA/wAAAAEAAAAEAAAAGQAAABwAAAOEAAAAFAD/////AQAAAAIA"}}]})");

        layoutProvider->registerLayout(
            R"({"Ember":[{"SplitterVertical":{"Children":[{"SplitterHorizontal":{"Children":[{"FilterWidget":{"Type":"Genre","Sort":"AscendingOrder"}},
            {"FilterWidget":{"Type":"Album
            Artist","Sort":"AscendingOrder"}},{"FilterWidget":{"Type":"Artist","Sort":"AscendingOrder"}},
            {"FilterWidget":{"Type":"Album","Sort":"AscendingOrder"}}],"State":"AAAA/wAAAAEAAAAFAAABAAAAAQAAAAEAAAABAAAAAQAA/////wEAAAABAA=="}},
            {"SplitterHorizontal":{"Children":["ControlBar","SearchBar"],"State":"AAAA/wAAAAEAAAADAAAFfgAAAdIAAAC1AP////8BAAAAAQA="}},
            {"SplitterHorizontal":{"Children":[{"SplitterVertical":{"Children":["ArtworkPanel","InfoPanel"],
            "State":"AAAA/wAAAAEAAAADAAABzAAAAbcAAAAUAP////8BAAAAAgA="}},"Playlist"],"State":"AAAA/wAAAAEAAAADAAABdQAABdsAAAC1AP////8BAAAAAQA="}},
            "StatusBar"],"State":"AAAA/wAAAAEAAAAFAAAA/wAAAB4AAALRAAAAGQAAAAAA/////wEAAAACAA=="}}]})");
    }
};

FiltersPlugin::FiltersPlugin()
    : p{std::make_unique<Private>()}
{ }

FiltersPlugin::~FiltersPlugin() = default;

void FiltersPlugin::initialise(const CorePluginContext& context)
{
    p->library       = context.library;
    p->playerManager = context.playerManager;
    p->settings      = context.settingsManager;

    p->filterSettings = std::make_unique<FiltersSettings>(p->settings);
}

void FiltersPlugin::initialise(const GuiPluginContext& context)
{
    p->actionManager    = context.actionManager;
    p->layoutProvider   = context.layoutProvider;
    p->searchController = context.searchController;
    p->factory          = context.widgetFactory;
    p->trackSelection   = context.trackSelection;

    p->filterManager = new FilterManager(p->library, p->trackSelection, p->settings, this);

    QObject::connect(p->searchController, &SearchController::searchChanged, p->filterManager,
                     &FilterManager::searchChanged);

    p->factory->registerClass<FilterWidget>(
        QStringLiteral("FilterWidget"), [this]() { return p->filterManager->createFilter(); }, "Filter Widget");

    p->generalPage = std::make_unique<FiltersGeneralPage>(p->settings);
    p->fieldsPage
        = std::make_unique<FiltersFieldsPage>(p->actionManager, p->filterManager->fieldRegistry(), p->settings);

    p->registerLayouts();
}

void FiltersPlugin::shutdown()
{
    p->filterManager->shutdown();
}
} // namespace Fooyin::Filters

#include "moc_filtersplugin.cpp"
