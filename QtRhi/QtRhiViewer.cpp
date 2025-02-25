#include "QtRhiViewer.h"
#include "QtMesh.h"

#include <rhi/qrhi.h>
#include <QFile>

QShader getShader(const QString& name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
}

QtRhiViewer::QtRhiViewer(QWidget* parent): QRhiWidget(parent) {}

QtRhiViewer::~QtRhiViewer() = default;

void QtRhiViewer::loadMesh(QString path)
{
    m_mesh = QtMesh::Load(path);
}

void QtRhiViewer::initialize(QRhiCommandBuffer* cb)
{
    assert(m_mesh);

    if (m_rhi != rhi())
    {
        m_rhi = rhi();
        rebuildScene();
        qInfo() << "RHI backend changed to" << m_rhi->backendName();
    }
}

void QtRhiViewer::render(QRhiCommandBuffer* cb)
{
    auto ru = m_scene.ru;
    if (ru) m_scene.ru = nullptr;

    cb->beginPass(renderTarget(), Qt::black, {1.0f, 0}, ru);
    cb->setGraphicsPipeline(m_scene.pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, width(), height()));
    // FIXME: always failed here, pipeline looks invalid
    cb->setShaderResources();

    const QRhiCommandBuffer::VertexInput vbufBinding(m_scene.vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_scene.ibuf.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(6);

    cb->endPass();
}

void QtRhiViewer::rebuildScene()
{
    m_scene = {};
    m_scene.vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                        sizeof(QtMesh::Vertex) * m_mesh->m_vertices.size()));
    m_scene.vbuf->create();
    m_scene.ibuf.reset(m_rhi->newBuffer(QRhiBuffer::Type::Immutable, QRhiBuffer::IndexBuffer,
                                        sizeof(QtMesh::Index) * m_mesh->m_indices.size()));
    m_scene.ibuf->create();

    m_scene.ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16 * sizeof(float)));
    m_scene.ubuf->create();

    m_scene.ru = m_rhi->nextResourceUpdateBatch();
    m_scene.ru->uploadStaticBuffer(m_scene.vbuf.get(), m_mesh->m_vertices.constData());
    m_scene.ru->uploadStaticBuffer(m_scene.ibuf.get(), m_mesh->m_indices.constData());
    m_scene.ru->updateDynamicBuffer(m_scene.ubuf.get(), 0, 16 * sizeof(float), m_transform.constData());

    m_scene.sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                            QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_scene.sampler->create();

    m_scene.pipeline.reset(m_rhi->newGraphicsPipeline());
    m_scene.pipeline->setDepthTest(true);
    m_scene.pipeline->setDepthWrite(true);
    m_scene.pipeline->setShaderStages({{QRhiShaderStage::Vertex, getShader(QLatin1String("://shader/vert.qsb"))},
                                       {QRhiShaderStage::Fragment, getShader(QLatin1String("://shader/frag.qsb"))}});

    QRhiVertexInputLayout input_layout;
    input_layout.setBindings({sizeof(QtMesh::Vertex), QRhiVertexInputBinding::PerVertex, 1});
    input_layout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, offsetof(QtMesh::Vertex, position)},
                                {0, 1, QRhiVertexInputAttribute::Float3, offsetof(QtMesh::Vertex, normal)},
                                {0, 2, QRhiVertexInputAttribute::Float3, offsetof(QtMesh::Vertex, tangent)},
                                {0, 3, QRhiVertexInputAttribute::Float3, offsetof(QtMesh::Vertex, bitangent)},
                                {0, 4, QRhiVertexInputAttribute::Float2, offsetof(QtMesh::Vertex, uv)}});

    m_scene.pipeline->setSampleCount(renderTarget()->sampleCount());
    m_scene.pipeline->setVertexInputLayout(input_layout);
    m_scene.pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());

    m_transform = m_rhi->clipSpaceCorrMatrix();
}
