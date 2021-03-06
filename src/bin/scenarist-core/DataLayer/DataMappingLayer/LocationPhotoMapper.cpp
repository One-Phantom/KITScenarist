#include "LocationPhotoMapper.h"

#include <Domain/Location.h>
#include <Domain/LocationPhoto.h>

#include <3rd_party/Helpers/ImageHelper.h>

#include <QBuffer>

using namespace DataMappingLayer;


namespace {
	const QString COLUMNS = " id, fk_location_id, photo, sort_order ";
	const QString TABLE_NAME = " locations_photo ";
}

LocationPhoto* LocationPhotoMapper::find(const Identifier& _id)
{
	return dynamic_cast<LocationPhoto*>(abstractFind(_id));
}

LocationPhotosTable* LocationPhotoMapper::findAll()
{
	return qobject_cast<LocationPhotosTable*>(abstractFindAll());
}

LocationPhotosTable* LocationPhotoMapper::findAllForLocation(const Identifier& _locationIdentifier)
{
	//
	// Фильтр по связанным локациям
	//
	QString filter = QString(" WHERE fk_location_id = %1 ").arg(_locationIdentifier.value());

	return qobject_cast<LocationPhotosTable*>(abstractFindAll(filter));
}

void LocationPhotoMapper::insert(LocationPhoto* _location)
{
	abstractInsert(_location);
}

void LocationPhotoMapper::update(LocationPhoto* _location)
{
	abstractUpdate(_location);
}

void LocationPhotoMapper::remove(LocationPhoto* _location)
{
	abstractDelete(_location);
}

QString LocationPhotoMapper::findStatement(const Identifier& _id) const
{
	QString findStatement =
			QString("SELECT " + COLUMNS +
					" FROM " + TABLE_NAME +
					" WHERE id = %1 "
					)
			.arg(_id.value());
	return findStatement;
}

QString LocationPhotoMapper::findAllStatement() const
{
	return "SELECT " + COLUMNS + " FROM  " + TABLE_NAME;
}

QString LocationPhotoMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
	QString insertStatement =
			QString("INSERT INTO " + TABLE_NAME +
					" (id, fk_location_id, photo, sort_order) "
					" VALUES(?, ?, ?, ?) "
					);

	LocationPhoto* photo = dynamic_cast<LocationPhoto*>(_subject);
	_insertValues.clear();
	_insertValues.append(photo->id().value());
	_insertValues.append(photo->location()->id().value());
	_insertValues.append(ImageHelper::bytesFromImage(photo->photo()));
	_insertValues.append(photo->sortOrder());

	return insertStatement;
}

QString LocationPhotoMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
	QString updateStatement =
			QString("UPDATE " + TABLE_NAME +
					" SET fk_location_id = ?, "
					" photo = ?, "
					" sort_order = ? "
					" WHERE id = ? "
					);

	LocationPhoto* photo = dynamic_cast<LocationPhoto*>(_subject);
	_updateValues.clear();
	_updateValues.append(photo->location()->id().value());
	_updateValues.append(ImageHelper::bytesFromImage(photo->photo()));
	_updateValues.append(photo->sortOrder());
	_updateValues.append(photo->id().value());

	return updateStatement;
}

QString LocationPhotoMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
	QString deleteStatement = "DELETE FROM " + TABLE_NAME + " WHERE id = ?";

	_deleteValues.clear();
	_deleteValues.append(_subject->id().value());

	return deleteStatement;
}

DomainObject* LocationPhotoMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
	//
	// Локации не загружаются чтобы не вызывать циклической инициилизации данных,
	// связывание фотографий с локациями осуществляется посредством метода findAllForLocation
	//
	Location* location = 0;
	const QPixmap photo = ImageHelper::imageFromBytes(_record.value("photo").toByteArray());
	const int sortOrder = _record.value("sort_order").toInt();

	return new LocationPhoto(_id, location, photo, sortOrder);
}

void LocationPhotoMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
	if (LocationPhoto* locationPhoto = dynamic_cast<LocationPhoto*>(_domainObject)) {
		const QPixmap photo = ImageHelper::imageFromBytes(_record.value("photo").toByteArray());
		locationPhoto->setPhoto(photo);

		const int sortOrder = _record.value("sort_order").toInt();
		locationPhoto->setSortOrder(sortOrder);
	}
}

DomainObjectsItemModel* LocationPhotoMapper::modelInstance()
{
	return new LocationPhotosTable;
}

LocationPhotoMapper::LocationPhotoMapper()
{
}
