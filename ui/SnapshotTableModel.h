#pragma once
#include <QAbstractTableModel>
#include <vector>
#include <glm/glm.hpp>

struct ObjectSnapshot;

class SnapshotTableModel : public QAbstractTableModel {
public:
    explicit SnapshotTableModel(QObject* parent = nullptr);

    void setSnapshots(const std::vector<ObjectSnapshot>& snaps);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
private:
    std::vector<ObjectSnapshot> snapshots;
};