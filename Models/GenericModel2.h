#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QDebug>
#include <unordered_map>

#include <functional>
#include <memory>

template <typename Underlying>
class ErasedPointerToDataMember
{
public:
	virtual ~ErasedPointerToDataMember() = default;

	virtual QVariant toVariant(const Underlying& obj) const = 0;
	virtual void fromVariant(Underlying& obj, const QVariant& v) = 0;
};

template <typename Underlying, typename MemberType>
class PointerToDataMember : public ErasedPointerToDataMember<Underlying>
{
public:
	PointerToDataMember(MemberType Underlying::*p)
		: mPtrToDataMember(p)
	{
	}

	QVariant toVariant(const Underlying& obj) const override
	{
		return QVariant::fromValue(obj.*mPtrToDataMember);
	}

	void fromVariant(Underlying& obj, const QVariant& v) override
	{
		obj.*mPtrToDataMember = v.value<MemberType>();
	}
private:
	MemberType Underlying::*mPtrToDataMember;
};


template <typename Underlying>
class ModelDataAccessor
{
public:
	template <typename MemberType>
	ModelDataAccessor(MemberType Underlying::*ptr)
		: mStorage(new PointerToDataMember<Underlying, MemberType>(ptr))
	{
	}

	QVariant toVariant(const Underlying& obj) const
	{
		return mStorage->toVariant(obj);
	}

	void fromVariant(Underlying& obj, const QVariant& v)
	{
		return mStorage->fromVariant(obj, v);
	}

private:
	std::shared_ptr<ErasedPointerToDataMember<Underlying>> mStorage;
};




template <class Underlying, class Columns>
class GenericModel2 : public QAbstractTableModel
{
public:
	using Mapping	= QHash<int, ModelDataAccessor<Underlying>>;

	GenericModel2(Mapping mapping, QObject* parent = nullptr)
		: QAbstractTableModel(parent)
		, mMapping(std::move(mapping))
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

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
	{
		if (index.row() < 0 || index.row() >= rowCount() || index.column() >= columnCount()
				|| (Qt::EditRole != role))
		{
			return false;
		}

		auto setter = mMapping.find(index.column());
		if (mMapping.cend() != setter)
		{
			(*setter).fromVariant(mData[index.row()], value);
			emit dataChanged(index, index);

			return true;
		}

		qWarning() << "Column:" << index.column() << "not found";

		return false;
	}

	QVariant data(const QModelIndex& index, int role) const override
	{
		if (index.row() < 0 || index.row() >= rowCount()
				|| index.column() >= columnCount()
				|| (Qt::DisplayRole != role && Qt::EditRole != role && Qt::ForegroundRole != role) )
		{
			return QVariant();
		}

		auto getter = mMapping.find(index.column());
		if (mMapping.cend() != getter)
		{
			return (*getter).toVariant(mData[index.row()]);
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
		mData.insert(std::next(mData.begin(), row), count, Underlying());
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

	void clear()
	{
		beginResetModel();
		mData.clear();
		endResetModel();
	}

protected:
	QVector<Underlying> mData;

private:
	Mapping mMapping;
};


