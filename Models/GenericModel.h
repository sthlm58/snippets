#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QDebug>

#include <functional>

template <class Underlying, class Columns, template <class...> class Container = QVector>
class GenericModel : public QAbstractTableModel
{
public:
	using ModelType = GenericModel<Underlying, Columns, Container>;
	using GetterMap	= QHash<int, std::function<QVariant(const Underlying&)>>;
	using SetterMap	= QHash<int, std::function<void(Underlying&, const QVariant& data)>>;

	using BackendType		= Container<Underlying>;
	using UnderlyingType	= Underlying;

	GenericModel(GetterMap getters, SetterMap setters, QObject* parent = nullptr)
		: QAbstractTableModel(parent)
		, mGetters(std::move(getters))
		, mSetters(std::move(setters))
	{

	}

	int rowCount(const QModelIndex& parent = {}) const override
	{
		Q_UNUSED(parent)
		return static_cast<int>(mData.size());
	}

	int columnCount(const QModelIndex& parent = {}) const override
	{
		Q_UNUSED(parent)
		return Columns::COLUMN_COUNT;
	}

	bool setData(const QModelIndex& index, const QVariant& value, int role) override
	{
		if (index.row() < 0 || index.row() >= rowCount() || index.column() >= columnCount()
				|| Qt::EditRole != role)
		{
			return false;
		}

		auto setter = mSetters.find(index.column());
		if (mSetters.cend() != setter)
		{
			(*setter)(mData[static_cast<typename BackendType::size_type>(index.row())], value);
			emit dataChanged(index, index);

			return true;
		}

		qWarning() << "Column:" << index.column() << "not found";

		return false;
	}

	QVariant data(const QModelIndex& index, int role) const override
	{
		if (index.row() < 0 || index.row() >= rowCount()
				|| index.column() >= columnCount() || Qt::DisplayRole != role)
		{
			return QVariant();
		}

		auto getter = mGetters.find(index.column());
		if (mGetters.cend() != getter)
		{
			return (*getter)(mData[static_cast<typename BackendType::size_type>(index.row())]);
		}

		qWarning() << "Column:" << index.column() << "not found";

		return QVariant();
	}

	bool insertRows(int row, int count, const QModelIndex& parent = {}) override
	{
		Q_UNUSED(parent)

		if (count <= 0)
		{
			return false;
		}

		beginInsertRows(parent, row, row + count - 1);
		while (count-- > 0)
		{
			mData.insert(std::next(mData.begin(), row), Underlying());
		}
		endInsertRows();

		return true;
	}

	bool removeRows(int row, int count, const QModelIndex& parent = {}) override
	{
		Q_UNUSED(parent)

		if (count <= 0)
		{
			return false;
		}

		const int last = row + count - 1;

		beginRemoveRows(parent, row, last);
		mData.erase(std::next(mData.begin(), row), std::next(mData.begin(), last + 1));
		endRemoveRows();

		return true;
	}

	virtual void duplicateRow(int row)
	{
		if (row < 0 || row >= rowCount())
		{
			return;
		}
		beginInsertRows(QModelIndex{}, row + 1, row + 1);
		mData.insert(std::next(mData.begin(), row + 1), mData.at(row));
		endInsertRows();
	}

	void reset(BackendType backend = {})
	{
		beginResetModel();
		mData = std::move(backend);
		endResetModel();
	}

protected:
	BackendType mData;

private:
	GetterMap mGetters;
	SetterMap mSetters;
};


