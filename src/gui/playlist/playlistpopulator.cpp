/*
 * Fooyin
 * Copyright © 2023, Luke Taylor <LukeT1@proton.me>
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

#include "playlistpopulator.h"

#include "playlistpreset.h"
#include "playlistscriptregistry.h"

#include <core/player/playercontroller.h>
#include <utils/crypto.h>

#include <QCryptographicHash>
#include <QTimer>

#include <ranges>

constexpr int TrackPreloadSize = 2000;

namespace Fooyin {
struct PlaylistPopulator::Private
{
    PlaylistPopulator* m_self;
    PlayerController* m_playerController;

    PlaylistPreset m_currentPreset;
    PlaylistColumnList m_columns;

    std::unique_ptr<PlaylistScriptRegistry> m_registry;
    ScriptParser m_parser;

    ScriptFormatter m_formatter;

    int m_trackDepth{0};
    QString m_prevBaseHeaderKey;
    QString m_prevHeaderKey;
    int m_prevIndex{0};
    std::vector<QString> m_prevBaseSubheaderKey;
    std::vector<QString> m_prevSubheaderKey;

    std::vector<PlaylistContainerItem> m_subheaders;

    PlaylistItem m_root;
    PendingData m_data;
    ContainerKeyMap m_headers;
    TrackList m_pendingTracks;

    explicit Private(PlaylistPopulator* self, PlayerController* playerController)
        : m_self{self}
        , m_playerController{playerController}
        , m_registry{std::make_unique<PlaylistScriptRegistry>()}
        , m_parser{m_registry.get()}
    { }

    void reset()
    {
        m_data.clear();
        m_headers.clear();
        m_trackDepth = 0;
        m_prevBaseSubheaderKey.clear();
        m_prevSubheaderKey.clear();
        m_prevBaseHeaderKey.clear();
        m_prevHeaderKey.clear();
    }

    PlaylistItem* getOrInsertItem(const QString& key, PlaylistItem::ItemType type, const Data& item,
                                  PlaylistItem* parent, const QString& baseKey)
    {
        auto [node, inserted] = m_data.items.try_emplace(key, PlaylistItem{type, item, parent});
        if(inserted) {
            node->second.setBaseKey(baseKey);
            node->second.setKey(key);
        }
        PlaylistItem* child = &node->second;

        if(!child->pending()) {
            child->setPending(true);
            m_data.nodes[parent->key()].push_back(key);
            if(type != PlaylistItem::Track) {
                m_data.containerOrder.push_back(key);
            }
        }
        return child;
    }

    void updateContainers()
    {
        for(const auto& [key, container] : m_headers) {
            container->updateGroupText(&m_parser, &m_formatter);
        }
    }

    void iterateHeader(const Track& track, PlaylistItem*& parent, int index)
    {
        HeaderRow row{m_currentPreset.header};
        if(!row.isValid()) {
            return;
        }

        auto evaluateBlocks = [this, track](RichScript& script) -> QString {
            script.text.clear();
            const auto evalScript = m_parser.evaluate(script.script, track);
            if(!evalScript.isEmpty()) {
                script.text = m_formatter.evaluate(evalScript);
            }
            return evalScript;
        };

        auto generateHeaderKey = [&row, &evaluateBlocks]() {
            return Utils::generateHash(evaluateBlocks(row.title), evaluateBlocks(row.subtitle),
                                       evaluateBlocks(row.sideText), evaluateBlocks(row.info));
        };

        const QString baseKey = generateHeaderKey();
        QString key           = Utils::generateRandomHash();
        if(!m_prevHeaderKey.isEmpty() && m_prevBaseHeaderKey == baseKey && index == m_prevIndex + 1) {
            key = m_prevHeaderKey;
        }
        m_prevBaseHeaderKey = baseKey;
        m_prevHeaderKey     = key;

        if(!m_headers.contains(key)) {
            PlaylistContainerItem header{m_currentPreset.header.simple};
            header.setTitle(row.title);
            header.setSubtitle(row.subtitle);
            header.setSideText(row.sideText);
            header.setInfo(row.info);
            header.setRowHeight(row.rowHeight);
            header.calculateSize();

            auto* headerItem      = getOrInsertItem(key, PlaylistItem::Header, header, parent, baseKey);
            auto& headerContainer = std::get<1>(headerItem->data());
            m_headers.emplace(key, &headerContainer);
        }
        PlaylistContainerItem* header = m_headers.at(key);
        header->addTrack(track);
        m_data.trackParents[track.id()].push_back(key);

        auto* headerItem = &m_data.items.at(key);
        parent           = headerItem;
        ++m_trackDepth;
    }

    void iterateSubheaders(const Track& track, PlaylistItem*& parent, int index)
    {
        for(auto& subheader : m_currentPreset.subHeaders) {
            const auto leftScript    = m_parser.evaluate(subheader.leftText.script, track);
            subheader.leftText.text  = m_formatter.evaluate(leftScript);
            const auto rightScript   = m_parser.evaluate(subheader.rightText.script, track);
            subheader.rightText.text = m_formatter.evaluate(rightScript);

            PlaylistContainerItem currentContainer{false};
            currentContainer.setTitle(subheader.leftText);
            currentContainer.setSubtitle(subheader.rightText);
            currentContainer.setRowHeight(subheader.rowHeight);
            currentContainer.calculateSize();
            m_subheaders.push_back(currentContainer);
        }

        const int subheaderCount = static_cast<int>(m_subheaders.size());
        m_prevSubheaderKey.resize(subheaderCount);
        m_prevBaseSubheaderKey.resize(subheaderCount);

        auto generateSubheaderKey = [](const PlaylistContainerItem& subheader) {
            QString subheaderKey;
            for(const auto& block : subheader.title().text) {
                subheaderKey += block.text;
            }
            for(const auto& block : subheader.subtitle().text) {
                subheaderKey += block.text;
            }
            return subheaderKey;
        };

        for(int i{0}; const auto& subheader : m_subheaders) {
            const QString subheaderKey = generateSubheaderKey(subheader);

            if(subheaderKey.isEmpty()) {
                m_prevBaseSubheaderKey[i].clear();
                m_prevSubheaderKey[i].clear();
                continue;
            }

            const QString baseKey = Utils::generateHash(parent->baseKey(), subheaderKey);
            QString key           = Utils::generateRandomHash();
            if(static_cast<int>(m_prevSubheaderKey.size()) > i && m_prevBaseSubheaderKey.at(i) == baseKey
               && index == m_prevIndex + 1) {
                key = m_prevSubheaderKey.at(i);
            }
            m_prevBaseSubheaderKey[i] = baseKey;
            m_prevSubheaderKey[i]     = key;

            if(!m_headers.contains(key)) {
                auto* subheaderItem      = getOrInsertItem(key, PlaylistItem::Subheader, subheader, parent, baseKey);
                auto& subheaderContainer = std::get<1>(subheaderItem->data());
                m_headers.emplace(key, &subheaderContainer);
            }
            PlaylistContainerItem* subheaderContainer = m_headers.at(key);
            subheaderContainer->addTrack(track);
            m_data.trackParents[track.id()].push_back(key);

            auto* subheaderItem = &m_data.items.at(key);
            parent              = subheaderItem;
            ++i;
            ++m_trackDepth;
        }

        m_subheaders.clear();
    }

    void evaluateTrackScript(RichScript& script, const Track& track)
    {
        script.text.clear();
        const auto evalScript = m_parser.evaluate(script.script, track);
        if(!evalScript.isEmpty()) {
            script.text = m_formatter.evaluate(evalScript);
        }
    }

    PlaylistItem* iterateTrack(const Track& track, int index)
    {
        PlaylistItem* parent = &m_root;

        iterateHeader(track, parent, index);
        iterateSubheaders(track, parent, index);

        if(!m_currentPreset.track.isValid()) {
            return nullptr;
        }

        m_registry->setTrackProperties(index, m_trackDepth);

        TrackRow trackRow{m_currentPreset.track};
        PlaylistTrackItem playlistTrack;

        if(!m_columns.empty()) {
            for(const auto& column : m_columns) {
                const auto evalScript = m_parser.evaluate(column.field, track);
                trackRow.columns.emplace_back(column.field, m_formatter.evaluate(evalScript));
            }
            playlistTrack = {trackRow.columns, track};
        }
        else {
            evaluateTrackScript(trackRow.leftText, track);
            evaluateTrackScript(trackRow.rightText, track);

            playlistTrack = {trackRow.leftText, trackRow.rightText, track};
        }

        playlistTrack.setRowHeight(trackRow.rowHeight);
        playlistTrack.setDepth(m_trackDepth);
        playlistTrack.calculateSize();

        const QString baseKey = Utils::generateHash(parent->key(), track.hash(), QString::number(index));
        const QString key     = Utils::generateRandomHash();

        auto* trackItem = getOrInsertItem(key, PlaylistItem::Track, playlistTrack, parent, baseKey);
        m_data.trackParents[track.id()].push_back(key);

        m_trackDepth = 0;
        m_prevIndex  = index;
        return trackItem;
    }

    void runBatch(int size, int index)
    {
        if(size <= 0) {
            return;
        }

        auto tracksBatch = std::ranges::views::take(m_pendingTracks, size);

        for(const Track& track : tracksBatch) {
            if(!m_self->mayRun()) {
                return;
            }
            iterateTrack(track, index++);
        }

        updateContainers();

        if(!m_self->mayRun()) {
            return;
        }

        emit m_self->populated(m_data);

        auto tracksToKeep = std::ranges::views::drop(m_pendingTracks, size);
        TrackList tempTracks;
        std::ranges::copy(tracksToKeep, std::back_inserter(tempTracks));
        m_pendingTracks = std::move(tempTracks);

        m_data.nodes.clear();

        const auto remaining = static_cast<int>(m_pendingTracks.size());
        runBatch(remaining, index);
    }

    void runTracksGroup(const std::map<int, TrackList>& tracks)
    {
        for(const auto& [index, trackGroup] : tracks) {
            std::vector<QString> trackKeys;

            int trackIndex{index};

            for(const Track& track : trackGroup) {
                if(!m_self->mayRun()) {
                    return;
                }
                if(const auto* trackItem = iterateTrack(track, trackIndex++)) {
                    trackKeys.push_back(trackItem->key());
                }
            }
            m_data.indexNodes.emplace(index, trackKeys);
        }

        updateContainers();

        if(!m_self->mayRun()) {
            return;
        }

        emit m_self->populatedTrackGroup(m_data);
    }
};

PlaylistPopulator::PlaylistPopulator(PlayerController* playerController, QObject* parent)
    : Worker{parent}
    , p{std::make_unique<Private>(this, playerController)}
{
    qRegisterMetaType<PendingData>();
}

void PlaylistPopulator::run(const Id& playlistId, const PlaylistPreset& preset, const PlaylistColumnList& columns,
                            const TrackList& tracks)
{
    setState(Running);

    p->reset();

    p->m_data.playlistId = playlistId;
    p->m_currentPreset   = preset;
    p->m_columns         = columns;
    p->m_pendingTracks   = tracks;
    p->m_registry->setup(playlistId, p->m_playerController->playbackQueue());

    p->runBatch(TrackPreloadSize, 0);

    emit finished();

    setState(Idle);
}

void PlaylistPopulator::runTracks(const Id& playlistId, const PlaylistPreset& preset, const PlaylistColumnList& columns,
                                  const std::map<int, TrackList>& tracks)
{
    setState(Running);

    p->reset();

    p->m_data.playlistId = playlistId;
    p->m_currentPreset   = preset;
    p->m_columns         = columns;
    p->m_registry->setup(playlistId, p->m_playerController->playbackQueue());

    p->runTracksGroup(tracks);

    setState(Idle);
}

void PlaylistPopulator::updateTracks(const Id& playlistId, const PlaylistPreset& preset,
                                     const PlaylistColumnList& columns, const TrackItemMap& tracks)
{
    setState(Running);

    p->m_currentPreset = preset;
    p->m_registry->setup(playlistId, p->m_playerController->playbackQueue());

    ItemList updatedTracks;

    for(const auto& [track, item] : tracks) {
        PlaylistTrackItem& trackData = std::get<0>(item.data());

        p->m_registry->setTrackProperties(item.index(), trackData.depth());

        if(!columns.empty()) {
            std::vector<RichScript> trackColumns;
            for(const auto& column : columns) {
                const auto evalScript = p->m_parser.evaluate(column.field, track);
                trackColumns.emplace_back(column.field, p->m_formatter.evaluate(evalScript));
            }
            trackData.setColumns(trackColumns);
        }
        else {
            RichScript trackLeft{preset.track.leftText};
            RichScript trackRight{preset.track.rightText};

            p->evaluateTrackScript(trackLeft, track);
            p->evaluateTrackScript(trackRight, track);

            trackData.setLeftRight(trackLeft, trackRight);
        }

        updatedTracks.push_back(item);
    }

    emit tracksUpdated(updatedTracks);

    setState(Idle);
}

void PlaylistPopulator::updateHeaders(const ItemList& headers)
{
    setState(Running);

    ItemKeyMap updatedHeaders;

    for(const PlaylistItem& item : headers) {
        PlaylistContainerItem& header = std::get<1>(item.data());
        header.updateGroupText(&p->m_parser, &p->m_formatter);
        updatedHeaders.emplace(item.key(), item);
    }

    emit headersUpdated(updatedHeaders);

    setState(Idle);
}

PlaylistPopulator::~PlaylistPopulator() = default;
} // namespace Fooyin

#include "moc_playlistpopulator.cpp"
