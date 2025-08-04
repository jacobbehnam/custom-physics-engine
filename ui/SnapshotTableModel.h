#pragma once
#include <QAbstractTableModel>
#include <vector>
#include <glm/glm.hpp>

struct ObjectSnapshot;

class SnapshotTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SnapshotTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void setSnapshots(const std::vector<ObjectSnapshot> &snaps);

private:
    std::vector<ObjectSnapshot> snapshots;
};