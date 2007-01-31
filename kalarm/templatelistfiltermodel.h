/*
 *  templatelistfiltermodel.h  -  proxy model class for lists of alarm templates
 *  Program:  kalarm
 *  Copyright © 2007 by David Jarvie <software@astrojar.org.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef TEMPLATELISTFILTERMODEL_H
#define TEMPLATELISTFILTERMODEL_H

#include "kalarm.h"

#include "eventlistmodel.h"


class TemplateListFilterModel : public EventListFilterModel
{
	public:
		enum {   // data columns
			TypeColumn, TemplateNameColumn,
			ColumnCount
		};

		explicit TemplateListFilterModel(EventListModel* baseModel, QObject* parent = 0)
		               : EventListFilterModel(baseModel, parent) {}
		void setTypeFilter(bool excludeCommandAlarms);
		virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
		virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;

	protected:
		virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;
		virtual bool filterAcceptsColumn(int sourceCol, const QModelIndex& sourceParent) const;

	private:
		bool mCmdFilter;
};

#endif // TEMPLATELISTFILTERMODEL_H

