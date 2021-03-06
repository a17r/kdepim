/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef AKONADICONSOLE_AGENTCONFIGMODEL_H
#define AKONADICONSOLE_AGENTCONFIGMODEL_H

#include <AkonadiCore/AgentInstance>

#include <QAbstractItemModel>
#include <QVector>

class QDBusInterface;

class AgentConfigModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AgentConfigModel(QObject *parent = Q_NULLPTR);
    ~AgentConfigModel();
    void setAgentInstance(const Akonadi::AgentInstance &instance);

    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void reload();
    void writeConfig();

private:
    QVector<QPair<QString, QVariant> > m_settings;
    QDBusInterface *m_interface;
};

#endif
