/*
 * Fooyin
 * Copyright © 2022, Luke Taylor <LukeT1@proton.me>
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

#pragma once

#include <gui/fywidget.h>

namespace Fooyin {
class ActionManager;
class SettingsManager;
class PlayerController;

class PlayerControl : public FyWidget
{
    Q_OBJECT

public:
    PlayerControl(ActionManager* actionManager, PlayerController* playerController, SettingsManager* settings,
                  QWidget* parent = nullptr);
    ~PlayerControl() override;

    QString name() const override;
    QString layoutName() const override;
    void layoutEditingMenu(QMenu* menu) override;
    void saveLayoutData(QJsonObject& layout) override;
    void loadLayoutData(const QJsonObject& layout) override;
    void finalise() override;

private:
    struct Private;
    std::unique_ptr<Private> p;
};
} // namespace Fooyin
