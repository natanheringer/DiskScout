#ifndef SORTPROXYMODEL_H
#define SORTPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDateTime>

// Roles provided by FileSystemModel for proper sorting
namespace FileRoles {
    enum {
        SizeRole = Qt::UserRole + 1,
        ModifiedRole,
        ContentsRole
    };
}

class SortProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SortProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {
        setDynamicSortFilter(true);
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override {
        if (!sourceModel() || !source_left.isValid() || !source_right.isValid()) {
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }
        const int col = source_left.column();
        if (col == 1) { // Size
            const QVariant av = sourceModel()->data(source_left, FileRoles::SizeRole);
            const QVariant bv = sourceModel()->data(source_right, FileRoles::SizeRole);
            const quint64 a = av.isValid() ? av.toULongLong() : 0ULL;
            const quint64 b = bv.isValid() ? bv.toULongLong() : 0ULL;
            return a < b;
        } else if (col == 2) { // Contents
            const int a = sourceModel()->data(source_left, FileRoles::ContentsRole).toInt();
            const int b = sourceModel()->data(source_right, FileRoles::ContentsRole).toInt();
            return a < b;
        } else if (col == 3) { // Modified
            const QVariant av = sourceModel()->data(source_left, FileRoles::ModifiedRole);
            const QVariant bv = sourceModel()->data(source_right, FileRoles::ModifiedRole);
            const QDateTime a = av.isValid() ? av.toDateTime() : QDateTime();
            const QDateTime b = bv.isValid() ? bv.toDateTime() : QDateTime();
            return a.toMSecsSinceEpoch() < b.toMSecsSinceEpoch();
        }
        // Name - fallback to display text
        const QString a = sourceModel()->data(source_left, Qt::DisplayRole).toString();
        const QString b = sourceModel()->data(source_right, Qt::DisplayRole).toString();
        return a.localeAwareCompare(b) < 0;
    }
};

#endif // SORTPROXYMODEL_H


