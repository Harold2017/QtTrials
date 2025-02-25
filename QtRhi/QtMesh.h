#pragma once

#include <QVector3D>
#include <QVector2D>
#include <QMatrix4x4>
#include <QVector>
#include <QSharedPointer>

struct QtMesh
{
    struct Vertex
    {
        QVector3D position;
        QVector3D normal;
        QVector3D tangent;
        QVector3D bitangent;
        QVector2D uv;
    };

    struct Material
    {
        QMap<QString, QVariant> properties;
    };

    struct SubMeshData
    {
        uint32_t vertices_offset;
        uint32_t vertices_num;
        uint32_t indices_offset;
        uint32_t indices_num;
        uint32_t material_index;
        QMatrix4x4 local_transfrom;
    };

    using Index = uint32_t;

    QVector<Vertex> m_vertices;
    QVector<Index> m_indices;
    QVector<SubMeshData> m_submeshes;
    QVector<QSharedPointer<Material>> m_materials;

    static QSharedPointer<QtMesh> Load(QString const& path);
};
