#include "SnapshotTableModel.h"
#include "graphics/interfaces/IPhysicsBody.h"

SnapshotTableModel::SnapshotTableModel(QObject* parent) : QAbstractTableModel(parent){}

void SnapshotTableModel::setSnapshots(const std::vector<ObjectSnapshot> &snaps) {
    beginResetModel();
    snapshots = snaps;
    endResetModel();
}

int SnapshotTableModel::rowCount(const QModelIndex &parent) const {
    return static_cast<int>(snapshots.size());
}

int SnapshotTableModel::columnCount(const QModelIndex &parent) const {
    return 7; // time, pos.x, pos.y, pos.z, vel.x, vel.y, vel.z
}

QVariant SnapshotTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return {};
    const auto& s = snapshots[index.row()];

    switch (index.column()) {
        case 0: return s.time;
        case 1: return s.position.x;
        case 2: return s.position.y;
        case 3: return s.position.z;
        case 4: return s.velocity.x;
        case 5: return s.velocity.y;
        case 6: return s.velocity.z;
    }
    return {};
}

QVariant SnapshotTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return {};
    switch (section) {
        case 0: return QStringLiteral("Time (s)");
        case 1: return QStringLiteral("Pos X");
        case 2: return QStringLiteral("Pos Y");
        case 3: return QStringLiteral("Pos Z");
        case 4: return QStringLiteral("Vel X");
        case 5: return QStringLiteral("Vel Y");
        case 6: return QStringLiteral("Vel Z");
    }
    return {};
}
